#include "MapCompiler.h"
#include "MapProject.h"
#include "SpriteDefs.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace editor {
namespace {

// --- Engine constants (mirror tools/bsp-to-bin.py) ---
constexpr uint8_t POLY_FLAG_AXIS_NONE = 0x18;
constexpr uint8_t POLY_FLAG_SWAPXY    = 0x40;

constexpr uint32_t MARKER_DEAD = 0xDEADBEEF;
constexpr uint32_t MARKER_CAFE = 0xCAFEBABE;

// --- Intermediate representations ---

struct PolyVert {
	uint8_t x, y, z;
	int8_t  s, t;  // stored as signed — matches Python (int8_t) decode
};

struct PolyRecord {
	std::array<PolyVert, 4> verts;
	int     tileNum;
	bool    isWall;
	uint8_t ownerCol;
	uint8_t ownerRow;
};

struct LineRecord {
	uint8_t x0, x1, y0, y1;
	uint8_t flags;
	uint8_t ownerCol;
	uint8_t ownerRow;
};

struct BSPNode {
	bool    isLeaf = false;
	uint8_t axis = 0;           // 0=X, 1=Y
	int     splitCoord = 0;     // world coord at tile boundary
	int     left = -1, right = -1;
	std::vector<int> polyIndices;
	std::vector<int> lineIndices;
	std::vector<uint16_t> tileSet;   // packed (row<<8)|col, kept sorted
	uint8_t minX = 0, minY = 0, maxX = 0, maxY = 0;
};

// --- Little-endian writers ---
inline void wByte(std::vector<uint8_t>& out, int v) { out.push_back(uint8_t(v & 0xFF)); }
inline void wU16 (std::vector<uint8_t>& out, int v) {
	out.push_back(uint8_t( v        & 0xFF));
	out.push_back(uint8_t((v >> 8)  & 0xFF));
}
inline void wI16 (std::vector<uint8_t>& out, int v) {
	// same bytes; just different intent
	out.push_back(uint8_t( v        & 0xFF));
	out.push_back(uint8_t((v >> 8)  & 0xFF));
}
inline void wI32 (std::vector<uint8_t>& out, int32_t v) {
	for (int i = 0; i < 4; ++i) out.push_back(uint8_t((v >> (8 * i)) & 0xFF));
}
inline void wMarker(std::vector<uint8_t>& out, uint32_t m) {
	for (int i = 0; i < 4; ++i) out.push_back(uint8_t((m >> (8 * i)) & 0xFF));
}

// -------------------------------------------------------------------------
// generateGeometry — port of generate_geometry()
// -------------------------------------------------------------------------
static void generateGeometry(const MapProject& p,
                             std::vector<PolyRecord>& polys,
                             std::vector<LineRecord>& lines)
{
	const int dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};

	for (int row = 0; row < MAP_SIZE; ++row) {
		for (int col = 0; col < MAP_SIZE; ++col) {
			const bool solid = p.isSolid(col, row);

			if (solid) {
				for (int di = 0; di < 4; ++di) {
					const int dc = dirs[di][0];
					const int dr = dirs[di][1];
					const int nc = col + dc;
					const int nr = row + dr;
					if (p.isSolid(nc, nr)) continue;

					// Wall edge in world coords, winding matching Python.
					int wx0, wy0, wx1, wy1;
					if (dc == 1) {
						wx0 = (col + 1) * 64; wy0 = row * 64;
						wx1 = (col + 1) * 64; wy1 = (row + 1) * 64;
					} else if (dc == -1) {
						wx0 = col * 64;       wy0 = (row + 1) * 64;
						wx1 = col * 64;       wy1 = row * 64;
					} else if (dr == 1) {
						wx0 = (col + 1) * 64; wy0 = (row + 1) * 64;
						wx1 = col * 64;       wy1 = (row + 1) * 64;
					} else {
						wx0 = col * 64;       wy0 = row * 64;
						wx1 = (col + 1) * 64; wy1 = row * 64;
					}

					const int bx0 = wx0 >> 3, by0 = wy0 >> 3;
					const int bx1 = wx1 >> 3, by1 = wy1 >> 3;
					const int bzl = p.floorByte(nc, nr);
					const int bzh = p.ceilByte(nc, nr);

					// Wall direction from the open neighbour's POV.
					// dc=+1 → open E of solid → wall on open's W; etc.
					int wallDir;
					if      (dc ==  1) wallDir = DIR_W;
					else if (dc == -1) wallDir = DIR_E;
					else if (dr ==  1) wallDir = DIR_N;
					else               wallDir = DIR_S;

					PolyRecord rec{};
					rec.verts[0] = { uint8_t(bx0), uint8_t(by0), uint8_t(bzl),  0,  0 };
					rec.verts[1] = { uint8_t(bx1), uint8_t(by1), uint8_t(bzl), 16,  0 };
					rec.verts[2] = { uint8_t(bx1), uint8_t(by1), uint8_t(bzh), 16, 16 };
					rec.verts[3] = { uint8_t(bx0), uint8_t(by0), uint8_t(bzh),  0, 16 };
					rec.tileNum = p.wallTex(nc, nr, wallDir);
					rec.isWall  = true;
					rec.ownerCol = uint8_t(nc);
					rec.ownerRow = uint8_t(nr);
					polys.push_back(rec);

					LineRecord ln{};
					int lx0 = wx0 >> 3, ly0 = wy0 >> 3;
					int lx1 = wx1 >> 3, ly1 = wy1 >> 3;
					lx0 = std::min(255, lx0); ly0 = std::min(255, ly0);
					lx1 = std::min(255, lx1); ly1 = std::min(255, ly1);
					ln.x0 = uint8_t(lx0); ln.x1 = uint8_t(lx1);
					ln.y0 = uint8_t(ly0); ln.y1 = uint8_t(ly1);
					ln.flags = p.isDoorTile(nc, nr) ? 4 : 0;
					ln.ownerCol = uint8_t(nc);
					ln.ownerRow = uint8_t(nr);
					lines.push_back(ln);
				}
			} else {
				// Floor quad
				const int bx0 = (col * 64) >> 3, by0 = (row * 64) >> 3;
				const int bx1 = ((col + 1) * 64) >> 3, by1 = ((row + 1) * 64) >> 3;
				const int bzf = p.floorByte(col, row);

				PolyRecord rec{};
				rec.verts[0] = { uint8_t(bx0), uint8_t(by0), uint8_t(bzf),  0,  0 };
				rec.verts[1] = { uint8_t(bx1), uint8_t(by0), uint8_t(bzf), 16,  0 };
				rec.verts[2] = { uint8_t(bx1), uint8_t(by1), uint8_t(bzf), 16, 16 };
				rec.verts[3] = { uint8_t(bx0), uint8_t(by1), uint8_t(bzf),  0, 16 };
				rec.tileNum = p.floorTex(col, row);
				rec.isWall  = false;
				rec.ownerCol = uint8_t(col);
				rec.ownerRow = uint8_t(row);
				polys.push_back(rec);

				// Ceiling quad (reversed winding)
				const int bzc = p.ceilByte(col, row);
				PolyRecord rec2{};
				rec2.verts[0] = { uint8_t(bx0), uint8_t(by1), uint8_t(bzc),  0, 16 };
				rec2.verts[1] = { uint8_t(bx1), uint8_t(by1), uint8_t(bzc), 16, 16 };
				rec2.verts[2] = { uint8_t(bx1), uint8_t(by0), uint8_t(bzc), 16,  0 };
				rec2.verts[3] = { uint8_t(bx0), uint8_t(by0), uint8_t(bzc),  0,  0 };
				rec2.tileNum = p.ceilTex(col, row);
				rec2.isWall  = false;
				rec2.ownerCol = uint8_t(col);
				rec2.ownerRow = uint8_t(row);
				polys.push_back(rec2);

				// Step walls with open +X/+Y neighbours (only those two to avoid
				// emitting each boundary twice).
				const int thisCeil  = bzc;
				const int thisFloor = bzf;
				const int steps[2][2] = {{1,0}, {0,1}};
				for (int si = 0; si < 2; ++si) {
					const int dc = steps[si][0];
					const int dr = steps[si][1];
					const int nc = col + dc;
					const int nr = row + dr;
					if (nc >= MAP_SIZE || nr >= MAP_SIZE) continue;
					if (p.isSolid(nc, nr)) continue;

					int swx0, swy0, swx1, swy1;
					if (dc == 1) {
						swx0 = (col + 1) * 64; swy0 = row * 64;
						swx1 = (col + 1) * 64; swy1 = (row + 1) * 64;
					} else {
						swx0 = (col + 1) * 64; swy0 = (row + 1) * 64;
						swx1 = col * 64;       swy1 = (row + 1) * 64;
					}
					const int sbx0 = swx0 >> 3, sby0 = swy0 >> 3;
					const int sbx1 = swx1 >> 3, sby1 = swy1 >> 3;

					// Default winding (sbx0/sby0 → sbx1/sby1 at bottom, then up to
					// zHigh) gives a normal in the +dc/+dr direction — i.e. facing
					// from the current tile into the neighbour. When the OWNER is
					// the current tile we need the normal flipped (so the face is
					// visible from inside the owner's BSP leaf, not through the
					// back face from the neighbour).
					auto emitStep = [&](int zLow, int zHigh, uint8_t oc, uint8_t orow,
					                    bool normalTowardCurrent) {
						PolyRecord sr{};
						if (!normalTowardCurrent) {
							sr.verts[0] = { uint8_t(sbx0), uint8_t(sby0), uint8_t(zLow),   0,  0 };
							sr.verts[1] = { uint8_t(sbx1), uint8_t(sby1), uint8_t(zLow),  16,  0 };
							sr.verts[2] = { uint8_t(sbx1), uint8_t(sby1), uint8_t(zHigh), 16, 16 };
							sr.verts[3] = { uint8_t(sbx0), uint8_t(sby0), uint8_t(zHigh),  0, 16 };
						} else {
							sr.verts[0] = { uint8_t(sbx1), uint8_t(sby1), uint8_t(zLow),   0,  0 };
							sr.verts[1] = { uint8_t(sbx0), uint8_t(sby0), uint8_t(zLow),  16,  0 };
							sr.verts[2] = { uint8_t(sbx0), uint8_t(sby0), uint8_t(zHigh), 16, 16 };
							sr.verts[3] = { uint8_t(sbx1), uint8_t(sby1), uint8_t(zHigh),  0, 16 };
						}
						sr.tileNum = D2_WALL_TILE;
						sr.isWall  = true;
						sr.ownerCol = oc;
						sr.ownerRow = orow;
						polys.push_back(sr);
					};

					// Ceiling step — raised ceiling is modelled as an alcove: the
					// step riser is only visible from INSIDE the taller-ceiling
					// tile (BSP owner), so from surrounding tiles the ceiling
					// reads as a flat line at the neighbour's height. Without
					// this, a single raised tile leaks a visible riser across
					// the whole room.
					const int neighCeil = p.ceilByte(nc, nr);
					if (neighCeil != thisCeil) {
						const int zLo = std::min(thisCeil, neighCeil);
						const int zHi = std::max(thisCeil, neighCeil);
						if (thisCeil > neighCeil) {
							// current has higher ceiling → owner=current, normal toward current
							emitStep(zLo, zHi, uint8_t(col), uint8_t(row), /*normalTowardCurrent=*/true);
						} else {
							// neighbour has higher ceiling → owner=neighbour, normal toward neighbour
							emitStep(zLo, zHi, uint8_t(nc),  uint8_t(nr),  /*normalTowardCurrent=*/false);
						}
					}
					// Floor step — keep owner on the LOWER-floor tile (the "step
					// up" is visually an obstacle looking from the low side), but
					// flip the winding so the visible face faces that owner.
					const int neighFloor = p.floorByte(nc, nr);
					if (neighFloor != thisFloor) {
						const int zLo = std::min(thisFloor, neighFloor);
						const int zHi = std::max(thisFloor, neighFloor);
						if (thisFloor < neighFloor) {
							emitStep(zLo, zHi, uint8_t(col), uint8_t(row), /*normalTowardCurrent=*/true);
						} else {
							emitStep(zLo, zHi, uint8_t(nc),  uint8_t(nr),  /*normalTowardCurrent=*/false);
						}
					}
				}
			}
		}
	}

	// --- Free-form lines: one wall quad + one collision line each. ---
	// Owner tile = the OPEN tile containing the line's midpoint. If the
	// midpoint is outside any open tile, the line is skipped (nowhere safe
	// to place it in a BSP leaf).
	for (const FreeLine& fl : p.freeLines) {
		int midX = (fl.x0 + fl.x1) / 2;
		int midY = (fl.y0 + fl.y1) / 2;
		int oc = midX / 64;
		int orow = midY / 64;
		if (oc < 0 || oc >= MAP_SIZE || orow < 0 || orow >= MAP_SIZE) continue;
		if (p.isSolid(oc, orow)) continue;

		const int bzl = p.floorByte(oc, orow);
		const int bzh = p.ceilByte(oc, orow);
		const int bx0 = std::min(255, fl.x0 >> 3);
		const int by0 = std::min(255, fl.y0 >> 3);
		const int bx1 = std::min(255, fl.x1 >> 3);
		const int by1 = std::min(255, fl.y1 >> 3);

		PolyRecord rec{};
		rec.verts[0] = { uint8_t(bx0), uint8_t(by0), uint8_t(bzl),  0,  0 };
		rec.verts[1] = { uint8_t(bx1), uint8_t(by1), uint8_t(bzl), 16,  0 };
		rec.verts[2] = { uint8_t(bx1), uint8_t(by1), uint8_t(bzh), 16, 16 };
		rec.verts[3] = { uint8_t(bx0), uint8_t(by0), uint8_t(bzh),  0, 16 };
		rec.tileNum = fl.texture;
		rec.isWall = true;
		rec.ownerCol = uint8_t(oc);
		rec.ownerRow = uint8_t(orow);
		polys.push_back(rec);

		LineRecord ln{};
		ln.x0 = uint8_t(bx0);
		ln.x1 = uint8_t(bx1);
		ln.y0 = uint8_t(by0);
		ln.y1 = uint8_t(by1);
		ln.flags = 0;
		ln.ownerCol = uint8_t(oc);
		ln.ownerRow = uint8_t(orow);
		lines.push_back(ln);
	}
}

// -------------------------------------------------------------------------
// buildBSP — port of build_bsp() (tile-centric, crossings-minimisation)
// -------------------------------------------------------------------------

using OpenSet = std::set<uint16_t>;   // packed (row<<8)|col; sorted for determinism

static bool hasOpenInRect(const OpenSet& open, int c0, int r0, int c1, int r1) {
	for (int r = r0; r < r1; ++r) {
		for (int c = c0; c < c1; ++c) {
			if (open.count(uint16_t((r << 8) | c))) return true;
		}
	}
	return false;
}

static int countCrossingsX(const OpenSet& open, int k, int r0, int r1) {
	int cnt = 0;
	for (int r = r0; r < r1; ++r) {
		if (open.count(uint16_t((r << 8) | (k-1))) && open.count(uint16_t((r << 8) | k))) cnt++;
	}
	return cnt;
}

static int countCrossingsY(const OpenSet& open, int k, int c0, int c1) {
	int cnt = 0;
	for (int c = c0; c < c1; ++c) {
		if (open.count(uint16_t(((k-1) << 8) | c)) && open.count(uint16_t((k << 8) | c))) cnt++;
	}
	return cnt;
}

static int buildBSP(const OpenSet& openTiles,
                    int c0, int r0, int c1, int r1,
                    int depth,
                    std::vector<BSPNode>& nodes)
{
	const int idx = int(nodes.size());
	nodes.emplace_back();
	BSPNode& node = nodes.back();

	// AABB in byte space
	node.minX = uint8_t(std::min(255, (c0 * TILE_SIZE) >> 3));
	node.minY = uint8_t(std::min(255, (r0 * TILE_SIZE) >> 3));
	node.maxX = uint8_t(std::min(255, (c1 * TILE_SIZE) >> 3));
	node.maxY = uint8_t(std::min(255, (r1 * TILE_SIZE) >> 3));

	// Leaf's tile set — iterate in sorted order for determinism.
	std::vector<uint16_t> ts;
	for (int r = r0; r < r1; ++r) {
		for (int c = c0; c < c1; ++c) {
			uint16_t k = uint16_t((r << 8) | c);
			if (openTiles.count(k)) ts.push_back(k);
		}
	}

	// Leaf termination
	if ((c1 - c0 <= 1 && r1 - r0 <= 1) || ts.empty()) {
		nodes[idx].isLeaf = true;
		nodes[idx].tileSet = std::move(ts);
		return idx;
	}

	// Split selection
	enum { AX_X = 0, AX_Y = 1 };
	const int preferred = (depth % 2 == 0) ? AX_X : AX_Y;
	const int other     = preferred == AX_X ? AX_Y : AX_X;

	struct Cand { int crossings; int distFromMid; int axis; int k; };
	bool haveBest = false;
	Cand best{};

	for (int pass = 0; pass < 2; ++pass) {
		const int axis = (pass == 0) ? preferred : other;

		int kBegin, kEnd;
		double mid;
		if (axis == AX_X) {
			if (c1 - c0 <= 1) continue;
			kBegin = c0 + 1; kEnd = c1;
			mid = (c0 + c1) / 2.0;
		} else {
			if (r1 - r0 <= 1) continue;
			kBegin = r0 + 1; kEnd = r1;
			mid = (r0 + r1) / 2.0;
		}

		bool axisHaveBest = false;
		Cand axisBest{};

		for (int k = kBegin; k < kEnd; ++k) {
			bool leftHas, rightHas;
			if (axis == AX_X) {
				leftHas  = hasOpenInRect(openTiles, c0, r0, k,  r1);
				rightHas = hasOpenInRect(openTiles, k,  r0, c1, r1);
			} else {
				leftHas  = hasOpenInRect(openTiles, c0, r0, c1, k);
				rightHas = hasOpenInRect(openTiles, c0, k,  c1, r1);
			}
			if (!(leftHas && rightHas)) continue;

			int crossings = (axis == AX_X)
				? countCrossingsX(openTiles, k, r0, r1)
				: countCrossingsY(openTiles, k, c0, c1);

			// Python uses `abs(k - mid)` where mid is a float; integer-k minus
			// .5-off mid yields a half-integer we compare as int*2 to avoid fp.
			int dist2x = std::abs(int(k * 2 - int((c0 + c1) * (axis == AX_X ? 1 : 0)
			                                  + (r0 + r1) * (axis == AX_Y ? 1 : 0))));
			// (Simpler correct form below.)
			(void)dist2x;
			int distFromMid = int(std::abs(double(k) - mid) * 2.0 + 0.5);  // *2 to keep integer

			Cand cur{ crossings, distFromMid, axis, k };
			bool better = !axisHaveBest
			              || std::tie(cur.crossings, cur.distFromMid)
			                     < std::tie(axisBest.crossings, axisBest.distFromMid);
			if (better) { axisBest = cur; axisHaveBest = true; }
		}

		if (axisHaveBest) {
			bool better = !haveBest
			              || std::tie(axisBest.crossings, axisBest.distFromMid)
			                     < std::tie(best.crossings, best.distFromMid);
			if (better) { best = axisBest; haveBest = true; }
			if (axis == preferred) break;  // commit to preferred axis
		}
	}

	if (!haveBest) {
		nodes[idx].isLeaf = true;
		nodes[idx].tileSet = std::move(ts);
		return idx;
	}

	nodes[idx].isLeaf = false;
	nodes[idx].axis = uint8_t(best.axis);
	nodes[idx].splitCoord = best.k * TILE_SIZE;

	int leftIdx, rightIdx;
	if (best.axis == AX_X) {
		leftIdx  = buildBSP(openTiles, c0, r0, best.k, r1, depth + 1, nodes);
		rightIdx = buildBSP(openTiles, best.k, r0, c1, r1, depth + 1, nodes);
	} else {
		leftIdx  = buildBSP(openTiles, c0, r0, c1, best.k, depth + 1, nodes);
		rightIdx = buildBSP(openTiles, c0, best.k, c1, r1, depth + 1, nodes);
	}
	nodes[idx].left = leftIdx;
	nodes[idx].right = rightIdx;
	return idx;
}

// -------------------------------------------------------------------------
// Attach polys/lines to leaves (by owner_tile lookup)
// -------------------------------------------------------------------------

static void attachPolysLinesToLeaves(std::vector<BSPNode>& nodes,
                                     const std::vector<PolyRecord>& polys,
                                     const std::vector<LineRecord>& lines)
{
	// tile → vector<polyIdx> (insertion order preserves generate_geometry order)
	std::map<uint16_t, std::vector<int>> tilePolys;
	for (int i = 0; i < int(polys.size()); ++i) {
		tilePolys[uint16_t((int(polys[i].ownerRow) << 8) | polys[i].ownerCol)].push_back(i);
	}
	std::map<uint16_t, std::vector<int>> tileLines;
	for (int i = 0; i < int(lines.size()); ++i) {
		tileLines[uint16_t((int(lines[i].ownerRow) << 8) | lines[i].ownerCol)].push_back(i);
	}

	for (BSPNode& n : nodes) {
		if (!n.isLeaf) continue;
		for (uint16_t t : n.tileSet) {
			auto pit = tilePolys.find(t);
			if (pit != tilePolys.end()) {
				n.polyIndices.insert(n.polyIndices.end(), pit->second.begin(), pit->second.end());
			}
			auto lit = tileLines.find(t);
			if (lit != tileLines.end()) {
				n.lineIndices.insert(n.lineIndices.end(), lit->second.begin(), lit->second.end());
			}
		}
	}
}

// -------------------------------------------------------------------------
// packPolys — per-leaf polygon data, grouped by texture
// -------------------------------------------------------------------------
static std::vector<uint8_t> packPolys(const std::vector<PolyRecord>& polys,
                                      const std::vector<int>& polyIndices)
{
	// Preserve insertion order for the texture group keys (like Python's OrderedDict).
	std::vector<int> textureOrder;
	std::map<int, std::vector<int>> byTex;
	for (int pi : polyIndices) {
		int tex = polys[pi].tileNum;
		if (!byTex.count(tex)) textureOrder.push_back(tex);
		byTex[tex].push_back(pi);
	}

	// Re-key to textureOrder's traversal for deterministic same-as-Python emission.
	std::vector<uint8_t> out;
	out.push_back(uint8_t(textureOrder.size()));

	for (int tex : textureOrder) {
		const auto& entries = byTex[tex];
		if (int(entries.size()) > 127) {
			throw std::runtime_error("packPolys: mesh exceeds 127 polys (tile="
			                         + std::to_string(tex) + ")");
		}
		// Reserved 4 bytes
		out.push_back(0); out.push_back(0); out.push_back(0); out.push_back(0);
		// (tileNum << 7) | polyCount
		int count = int(entries.size());
		int packed = ((tex & 0x1FF) << 7) | (count & 0x7F);
		out.push_back(uint8_t(packed & 0xFF));
		out.push_back(uint8_t((packed >> 8) & 0xFF));

		for (int pi : entries) {
			const PolyRecord& rec = polys[pi];
			const int numVerts = 4;
			uint8_t flags = uint8_t(((numVerts - 2) & 0x7) | POLY_FLAG_AXIS_NONE);
			if (rec.isWall) flags |= POLY_FLAG_SWAPXY;
			out.push_back(flags);
			for (const PolyVert& v : rec.verts) {
				out.push_back(v.x);
				out.push_back(v.y);
				out.push_back(v.z);
				out.push_back(uint8_t(v.s));  // bit pattern of signed int8
				out.push_back(uint8_t(v.t));
			}
		}
	}
	return out;
}

// -------------------------------------------------------------------------
// writeBin — assemble all sections into the final file bytes
// -------------------------------------------------------------------------
static std::vector<uint8_t> writeBin(const MapProject& p,
                                     const std::vector<PolyRecord>& polys,
                                     const std::vector<LineRecord>& lines,
                                     std::vector<BSPNode>& nodes,
                                     int& outNumPolys,
                                     int& outNumLines,
                                     int& outNumNodes,
                                     int& outNumLeaves,
                                     std::set<int>& outMediaSet)
{
	// --- Per-leaf constraint checks ---
	int leafCount = 0;
	for (const BSPNode& n : nodes) if (n.isLeaf) ++leafCount;
	if (leafCount > 256) {
		throw std::runtime_error("BSP has " + std::to_string(leafCount)
		                         + " leaves (engine limit 256)");
	}
	for (size_t i = 0; i < nodes.size(); ++i) {
		const BSPNode& n = nodes[i];
		if (n.isLeaf && int(n.lineIndices.size()) > 63) {
			throw std::runtime_error("leaf " + std::to_string(i)
			                         + " has " + std::to_string(n.lineIndices.size())
			                         + " collision lines (max 63)");
		}
	}

	// --- Lines in leaf-contiguous order ---
	std::vector<LineRecord> allLinesOrdered;
	std::vector<int> leafLineStart(nodes.size(), 0);
	std::vector<int> leafLineCount(nodes.size(), 0);
	for (size_t i = 0; i < nodes.size(); ++i) {
		if (!nodes[i].isLeaf) continue;
		leafLineStart[i] = int(allLinesOrdered.size());
		leafLineCount[i] = int(nodes[i].lineIndices.size());
		if (leafLineStart[i] > 0x3FF) {
			throw std::runtime_error("leaf " + std::to_string(i) + " lineStart exceeds 10-bit field");
		}
		for (int li : nodes[i].lineIndices) allLinesOrdered.push_back(lines[li]);
	}

	// --- Pack poly data per leaf ---
	std::vector<int> leafPolyData(nodes.size(), 0);
	std::vector<uint8_t> allPolyData;
	for (size_t i = 0; i < nodes.size(); ++i) {
		if (!nodes[i].isLeaf) continue;
		leafPolyData[i] = int(allPolyData.size());
		auto packed = packPolys(polys, nodes[i].polyIndices);
		allPolyData.insert(allPolyData.end(), packed.begin(), packed.end());
	}
	if (allPolyData.size() > 0xFFFF) {
		throw std::runtime_error("dataSizePolys exceeds u16");
	}

	// --- Normals ---
	constexpr int numNormals = 2;
	const int16_t normals[numNormals][3] = {
		{-16384, 0, 0},
		{0, -16384, 0},
	};

	const int numNodes = int(nodes.size());
	const int numLines = int(allLinesOrdered.size());
	const int dataSizePolys = int(allPolyData.size());

	// --- Per-node arrays ---
	std::vector<uint16_t> nodeOffsets(numNodes);
	std::vector<uint8_t>  nodeNormalIdxs(numNodes);
	std::vector<uint16_t> nodeChild1(numNodes);
	std::vector<uint16_t> nodeChild2(numNodes);
	std::vector<uint8_t>  nodeBounds(numNodes * 4);
	for (int i = 0; i < numNodes; ++i) {
		const BSPNode& n = nodes[i];
		nodeBounds[i * 4 + 0] = n.minX;
		nodeBounds[i * 4 + 1] = n.minY;
		nodeBounds[i * 4 + 2] = n.maxX;
		nodeBounds[i * 4 + 3] = n.maxY;
		if (n.isLeaf) {
			nodeOffsets[i]    = 0xFFFF;
			nodeNormalIdxs[i] = 0;
			nodeChild1[i]     = uint16_t(leafPolyData[i] & 0xFFFF);
			int ls = leafLineStart[i];
			int lc = leafLineCount[i];
			nodeChild2[i]     = uint16_t((ls & 0x3FF) | ((lc & 0x3F) << 10));
		} else {
			nodeNormalIdxs[i] = n.axis;
			nodeOffsets[i]    = uint16_t((n.splitCoord * 16) & 0xFFFF);
			nodeChild1[i]     = uint16_t(n.left  & 0xFFFF);
			nodeChild2[i]     = uint16_t(n.right & 0xFFFF);
		}
	}

	// --- Line arrays ---
	std::vector<uint8_t> lineFlagsArr((numLines + 1) / 2, 0);
	std::vector<uint8_t> lineXs(numLines * 2);
	std::vector<uint8_t> lineYs(numLines * 2);
	for (int i = 0; i < numLines; ++i) {
		const LineRecord& l = allLinesOrdered[i];
		lineXs[i * 2 + 0] = l.x0;
		lineXs[i * 2 + 1] = l.x1;
		lineYs[i * 2 + 0] = l.y0;
		lineYs[i * 2 + 1] = l.y1;
		uint8_t nibble = l.flags & 0x0F;
		if (i & 1) lineFlagsArr[i >> 1] |= uint8_t(nibble << 4);
		else       lineFlagsArr[i >> 1] |= nibble;
	}

	// --- Height map — constant FLOOR_HEIGHT in the .bin. Per-tile values are
	// supplied by level.yaml `height_map:` (engine override at
	// src/engine/render/Render.cpp:1021), not the binary heightMap.
	std::vector<uint8_t> heightMap(1024, uint8_t(FLOOR_HEIGHT));

	// --- Map flags (nibble-packed block_map, with door tiles' solid bit cleared) ---
	std::vector<uint8_t> mapFlags512(512, 0);
	for (int i = 0; i < 512; ++i) {
		int ta = i * 2, tb = i * 2 + 1;
		int fa = (ta < 1024) ? (p.blockMap[ta] & 0x0F) : 0;
		int fb = (tb < 1024) ? (p.blockMap[tb] & 0x0F) : 0;
		mapFlags512[i] = uint8_t((fa & 0x0F) | ((fb & 0x0F) << 4));
	}
	for (const Door& d : p.doors) {
		int tidx = d.row * 32 + d.col;
		if (tidx < 0 || tidx >= 1024) continue;
		int byteIdx = tidx / 2;
		if (tidx & 1) mapFlags512[byteIdx] &= 0x0F;
		else          mapFlags512[byteIdx] &= 0xF0;
	}

	// --- Media indices: defaults + every tileNum actually referenced by a poly.
	// Otherwise a texture the editor painted that isn't one of the three
	// defaults would crash Render::setupTexture at draw time.
	std::set<int> mediaSet;
	mediaSet.insert(D2_WALL_TILE);
	mediaSet.insert(D2_FLOOR_TILE);
	mediaSet.insert(D2_CEIL_TILE);
	for (const PolyRecord& pr : polys) mediaSet.insert(pr.tileNum);
	// Placed entities (monsters/items/NPCs) also need their sprite tile media
	// registered — otherwise the renderer crashes in setupTexture when the BSP
	// walker reaches the sprite. Entity ID 0 = unresolved (legacy YAML); skip.
	// Multi-layer monsters (e.g. Revenant, Cyberdemon) render as stacked
	// composite sprites; each layer is a distinct sprite ID that must also
	// be registered, or the renderer drops those layers and the monster
	// appears partially drawn (legs only, torso missing).
	auto addEntityMedia = [&](int id) {
		if (id <= 0) return;
		mediaSet.insert(id);
		if (auto* rp = SpriteDefs::getRenderProps(id)) {
			for (const auto& layer : rp->composite) {
				if (layer.sprite > 0) mediaSet.insert(layer.sprite);
			}
			if (rp->glow.sprite > 0) mediaSet.insert(rp->glow.sprite);
		}
	};
	for (const Entity& e : p.entities) addEntityMedia(e.tileId);
	std::vector<int> mediaIndices(mediaSet.begin(), mediaSet.end());

	// --- Spawn ---
	int spawnIndex = p.spawn.row * MAP_SIZE + p.spawn.col;
	int d2SpawnDir = (p.spawn.dir / 32) & 7;   // match Python (dir stored 0..7 here; Python stores 0..255)
	// NOTE: Python treats spawn_dir as the raw 0..255 byte; we store 0..7 already,
	// so apply the same conversion to stay bit-identical.

	// --- Assemble .bin ---
	std::vector<uint8_t> out;

	// Header (42 bytes)
	wByte(out, 3);                              // version
	wI32 (out, 0x44314250);                     // "D1BP"
	wU16 (out, spawnIndex);                     // mapSpawnIndex
	wByte(out, d2SpawnDir);                     // mapSpawnDir
	wByte(out, 0);                              // mapFlagsBitmask
	wByte(out, 0);                              // totalSecrets
	wByte(out, 0);                              // totalLoot
	wU16 (out, numNodes);
	wU16 (out, dataSizePolys);
	wU16 (out, numLines);
	wU16 (out, numNormals);
	wU16 (out, 1);                              // numNormalSprites (1 invisible dummy)
	wI16 (out, 0);                              // numZSprites
	wI16 (out, 0);                              // numTileEvents
	wI16 (out, 0);                              // mapByteCodeSize
	wByte(out, 0);                              // totalMayaCameras
	wI16 (out, 0);                              // totalMayaCameraKeys
	for (int i = 0; i < 6; ++i) wI16(out, -1);  // mayaTweenOffsets

	// 1. Media
	wMarker(out, MARKER_DEAD);
	wU16 (out, int(mediaIndices.size()));
	for (int idx : mediaIndices) wU16(out, idx);
	wMarker(out, MARKER_DEAD);

	// 2. Normals
	for (int i = 0; i < numNormals; ++i) {
		wI16(out, normals[i][0]);
		wI16(out, normals[i][1]);
		wI16(out, normals[i][2]);
	}
	wMarker(out, MARKER_CAFE);

	// 3. Node offsets
	for (uint16_t v : nodeOffsets) wU16(out, v);
	wMarker(out, MARKER_CAFE);

	// 4. Node normal indices
	for (uint8_t v : nodeNormalIdxs) wByte(out, v);
	wMarker(out, MARKER_CAFE);

	// 5. Node child offsets (child1 followed by child2)
	for (uint16_t v : nodeChild1) wU16(out, v);
	for (uint16_t v : nodeChild2) wU16(out, v);
	wMarker(out, MARKER_CAFE);

	// 6. Node bounds
	out.insert(out.end(), nodeBounds.begin(), nodeBounds.end());
	wMarker(out, MARKER_CAFE);

	// 7. Node polygons
	out.insert(out.end(), allPolyData.begin(), allPolyData.end());
	wMarker(out, MARKER_CAFE);

	// 8. Line data
	out.insert(out.end(), lineFlagsArr.begin(), lineFlagsArr.end());
	out.insert(out.end(), lineXs.begin(), lineXs.end());
	out.insert(out.end(), lineYs.begin(), lineYs.end());
	wMarker(out, MARKER_CAFE);

	// 9. Height map
	out.insert(out.end(), heightMap.begin(), heightMap.end());
	wMarker(out, MARKER_CAFE);

	// 10-14. Sprites (one invisible dummy at spawn — matches Python)
	int spawnTx = (spawnIndex % 32) * 8 + 4;
	int spawnTy = (spawnIndex / 32) * 8 + 4;
	wByte(out, spawnTx);
	wByte(out, spawnTy);
	wByte(out, 0);                              // spriteInfoLow
	wMarker(out, MARKER_CAFE);
	wU16 (out, 0x0001);                         // spriteInfoHigh (invisible)
	wMarker(out, MARKER_CAFE);
	wMarker(out, MARKER_CAFE);                  // Z-sprite heights (empty)
	wMarker(out, MARKER_CAFE);                  // Z-sprite info mid (empty)

	// 15. Static functions (12 × 0xFFFF sentinels)
	for (int i = 0; i < 12; ++i) wU16(out, 0xFFFF);
	wMarker(out, MARKER_CAFE);

	// 16. Tile events (empty)
	wMarker(out, MARKER_CAFE);

	// 17. Bytecode (empty)
	wMarker(out, MARKER_CAFE);

	// 18. Maya cameras (empty)
	wMarker(out, MARKER_DEAD);

	// 19. Map flags
	out.insert(out.end(), mapFlags512.begin(), mapFlags512.end());
	wMarker(out, MARKER_CAFE);

	outNumPolys = int(polys.size());
	outNumLines = numLines;
	outNumNodes = numNodes;
	outNumLeaves = leafCount;
	outMediaSet = std::move(mediaSet);
	return out;
}

// -------------------------------------------------------------------------
// writeLevelYaml — minimal level.yaml matching write_level_yaml in the
// reference generator (key `entities:`, per-tile height_map, sky texture)
// -------------------------------------------------------------------------
static std::string writeLevelYaml(const MapProject& p,
                                  const std::vector<PolyRecord>& polys) {
	static const char* dirNames[] = {
		"east", "northeast", "north", "northwest",
		"west", "southwest", "south", "southeast"
	};

	std::ostringstream os;
	os << "# Level generated by editor::compileMap\n";
	os << "name: " << (p.name.empty() ? "unnamed" : p.name) << "\n\n";
	os << "player_spawn:\n";
	os << "  x: " << int(p.spawn.col) << "\n";
	os << "  y: " << int(p.spawn.row) << "\n";
	int d2Dir = (p.spawn.dir / 32) & 7;
	os << "  dir: " << dirNames[d2Dir] << "\n\n";

	// Textures: emit every tileNum any poly references + the 3 hardcoded
	// defaults + door_unlocked (symbolic name resolved via SpriteDefs).
	// The engine's beginLoadMap treats this list as an OVERRIDE of the
	// .bin's media section (Render.cpp:822–839), so omissions cause
	// setupTexture to crash at draw time.
	//
	// Emit raw integer IDs: the engine accepts them directly (first char
	// is a digit → asInt()). Using `texture_<id>` symbolic names would
	// silently skip IDs that lack an entry in sprites.yaml (only ~252
	// texture_N names are defined across the 105–488 range).
	os << "textures:\n";
	std::set<int> texSet;
	texSet.insert(D2_WALL_TILE);
	texSet.insert(D2_FLOOR_TILE);
	texSet.insert(D2_CEIL_TILE);
	for (const PolyRecord& pr : polys) texSet.insert(pr.tileNum);
	// Include entity sprite IDs so loadMapLevelOverrides registers their media.
	// Without this, the textures-YAML override wipes the .bin's registration
	// and setupTexture segfaults when the renderer visits the sprite.
	// Composite layers and glow overlays are distinct sprite IDs; missing any
	// one of them leaves the corresponding layer of a multi-part monster
	// unrendered (e.g. torso missing while legs render).
	auto addEntityTex = [&](int id) {
		if (id <= 0) return;
		texSet.insert(id);
		if (auto* rp = SpriteDefs::getRenderProps(id)) {
			for (const auto& layer : rp->composite) {
				if (layer.sprite > 0) texSet.insert(layer.sprite);
			}
			if (rp->glow.sprite > 0) texSet.insert(rp->glow.sprite);
		}
	};
	for (const Entity& e : p.entities) addEntityTex(e.tileId);
	for (int t : texSet) os << "  - " << t << "\n";
	os << "  - door_unlocked\n\n";
	os << "sky_texture: " << p.skyTexture << "\n\n";
	os << "height_map:\n";
	for (int r = 0; r < MAP_SIZE; ++r) {
		os << "  - [";
		for (int c = 0; c < MAP_SIZE; ++c) {
			if (c) os << ", ";
			os << int(p.floorByte(c, r));
		}
		os << "]\n";
	}

	const bool anyEntity = !p.doors.empty() || !p.entities.empty();
	if (anyEntity) {
		os << "\nentities:\n";

		// Doors: same as before.
		for (const Door& d : p.doors) {
			int mx = d.col * TILE_SIZE + TILE_SIZE / 2;
			int my = d.row * TILE_SIZE + TILE_SIZE / 2;
			const char* dirFlags = (d.axis == 'V') ? "north, south" : "east, west";
			os << "  - x: " << mx << "\n";
			os << "    y: " << my << "\n";
			os << "    tile: door_unlocked\n";
			os << "    flags: [animation, " << dirFlags << "]\n";
		}

		// User-placed entities (monsters / items / NPCs / decorations).
		for (const Entity& e : p.entities) {
			int mx = e.col * TILE_SIZE + TILE_SIZE / 2;
			int my = e.row * TILE_SIZE + TILE_SIZE / 2;
			os << "  - x: " << mx << "\n";
			os << "    y: " << my << "\n";
			os << "    tile: " << (e.tile.empty() ? "unknown" : e.tile) << "\n";
			if (e.z >= 0) os << "    z: " << e.z << "\n";
			if (!e.flags.empty()) {
				os << "    flags: [";
				for (size_t i = 0; i < e.flags.size(); ++i) {
					if (i) os << ", ";
					os << e.flags[i];
				}
				os << "]\n";
			}
		}
	}

	return os.str();
}

}  // namespace

// -------------------------------------------------------------------------
// Public entry point
// -------------------------------------------------------------------------
CompiledMap compileMap(const MapProject& project) {
	std::vector<PolyRecord> polys;
	std::vector<LineRecord> lines;
	generateGeometry(project, polys, lines);

	OpenSet openTiles;
	for (int r = 0; r < MAP_SIZE; ++r) {
		for (int c = 0; c < MAP_SIZE; ++c) {
			if (!project.isSolid(c, r)) openTiles.insert(uint16_t((r << 8) | c));
		}
	}

	std::vector<BSPNode> nodes;
	buildBSP(openTiles, 0, 0, MAP_SIZE, MAP_SIZE, 0, nodes);
	attachPolysLinesToLeaves(nodes, polys, lines);

	CompiledMap result;
	result.binData = writeBin(project, polys, lines, nodes,
	                          result.numPolys, result.numLines,
	                          result.numNodes, result.numLeaves,
	                          result.mediaSet);
	result.levelYaml = writeLevelYaml(project, polys);
	return result;
}

}  // namespace editor
