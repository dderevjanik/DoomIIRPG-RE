#include <cstdlib>
#include <cstring>
#include <print>
#include <stdexcept>
#include <al.h>
#include <alc.h>
#include "Log.h"

#include "App.h"
#include "CAppContainer.h"
#include "Sound.h"
#include "Sounds.h"
#include "JavaStream.h"

#if defined(DRPG_HAS_VORBIS)
#include <vorbis/vorbisfile.h>
#endif

// Forward decl from inside this file — defined before MusicStream below.
static bool parseWavHeader(InputStream* is, ALenum* alFormat, int* sampleRate, int* dataOffset, int* dataSize);

// ---------------------------------------------------------------------------
// OggDecoder — pimpl-style state for Vorbis decoding. Lives only in this TU
// so the rest of the engine doesn't pull in vorbis headers.
// ---------------------------------------------------------------------------

#if defined(DRPG_HAS_VORBIS)
struct OggDecoder {
	OggVorbis_File vf;
	InputStream* stream = nullptr; // borrowed from MusicStream; not owned
	int cursor = 0;
	bool opened = false;

	OggDecoder() { std::memset(&vf, 0, sizeof(vf)); }
	~OggDecoder() {
		if (opened) {
			ov_clear(&vf);
		}
	}
};

// libvorbisfile callbacks adapting to InputStream::readChunk.
static size_t ovRead(void* ptr, size_t size, size_t nmemb, void* datasource) {
	OggDecoder* d = (OggDecoder*)datasource;
	int wanted = (int)(size * nmemb);
	int avail = d->stream->getFileSize() - d->cursor;
	int n = std::min(wanted, std::max(0, avail));
	if (n <= 0) return 0;
	if (!d->stream->readChunk((uint8_t*)ptr, d->cursor, n)) return 0;
	d->cursor += n;
	return (size_t)(n / size);
}

static int ovSeek(void* datasource, ogg_int64_t offset, int whence) {
	OggDecoder* d = (OggDecoder*)datasource;
	int newCursor = d->cursor;
	switch (whence) {
		case SEEK_SET: newCursor = (int)offset; break;
		case SEEK_CUR: newCursor = d->cursor + (int)offset; break;
		case SEEK_END: newCursor = d->stream->getFileSize() + (int)offset; break;
		default: return -1;
	}
	if (newCursor < 0 || newCursor > d->stream->getFileSize()) return -1;
	d->cursor = newCursor;
	return 0;
}

static long ovTell(void* datasource) {
	OggDecoder* d = (OggDecoder*)datasource;
	return d->cursor;
}

static int ovClose(void*) { return 0; } // we don't close — InputStream owns the file handle

static const ov_callbacks ovCallbacks = { ovRead, ovSeek, ovClose, ovTell };
#else
struct OggDecoder {}; // stub when vorbis is unavailable
#endif

// ---------------------------------------------------------------------------
// MusicStream — see Sound.h for description.
// ---------------------------------------------------------------------------

Sound::MusicStream::MusicStream() = default;

Sound::MusicStream::~MusicStream() {
	for (ALuint buf : streamBuffers) {
		if (buf != 0) {
			alDeleteBuffers(1, &buf);
		}
	}
}

// Parse a 16- or 18-byte "fmt " WAV chunk into format/freq/channels/bits and
// then walk the file looking for the "data" chunk. On success, populates
// dataOffset/dataSize and returns true. Works against any InputStream — file
// or in-memory buffer (from loadFromBuffer).
static bool parseWavHeader(InputStream* is, ALenum* alFormat, int* sampleRate, int* dataOffset, int* dataSize) {
	uint8_t hdr[44];
	if (!is->readChunk(hdr, 0, 12)) return false;
	if (std::memcmp(hdr, "RIFF", 4) != 0) return false;
	if (std::memcmp(hdr + 8, "WAVE", 4) != 0) return false;

	int pos = 12;
	int channels = 0, bits = 0;
	bool haveFmt = false;
	while (pos + 8 <= is->getFileSize()) {
		uint8_t chunkHdr[8];
		if (!is->readChunk(chunkHdr, pos, 8)) return false;
		uint32_t chunkLen = (uint32_t)chunkHdr[4] | ((uint32_t)chunkHdr[5] << 8)
		                  | ((uint32_t)chunkHdr[6] << 16) | ((uint32_t)chunkHdr[7] << 24);
		pos += 8;

		if (std::memcmp(chunkHdr, "fmt ", 4) == 0) {
			if (chunkLen < 16) return false;
			uint8_t fmt[18] = {};
			if (!is->readChunk(fmt, pos, (int)std::min<uint32_t>(chunkLen, 18))) return false;
			uint16_t fmtId = (uint16_t)fmt[0] | ((uint16_t)fmt[1] << 8);
			if (fmtId != 1) return false; // PCM only
			channels = (uint16_t)fmt[2] | ((uint16_t)fmt[3] << 8);
			*sampleRate = (int)((uint32_t)fmt[4] | ((uint32_t)fmt[5] << 8)
			                  | ((uint32_t)fmt[6] << 16) | ((uint32_t)fmt[7] << 24));
			bits = (uint16_t)fmt[14] | ((uint16_t)fmt[15] << 8);
			haveFmt = true;
		} else if (std::memcmp(chunkHdr, "data", 4) == 0) {
			if (!haveFmt) return false;
			*dataOffset = pos;
			*dataSize = (int)chunkLen;
			if (channels == 1 && bits == 8)  *alFormat = AL_FORMAT_MONO8;
			else if (channels == 1 && bits == 16) *alFormat = AL_FORMAT_MONO16;
			else if (channels == 2 && bits == 8)  *alFormat = AL_FORMAT_STEREO8;
			else if (channels == 2 && bits == 16) *alFormat = AL_FORMAT_STEREO16;
			else return false;
			return true;
		}
		pos += (int)chunkLen + (chunkLen & 1u); // chunks are 2-byte aligned
	}
	return false;
}

int Sound::MusicStream::readNextWavChunk(uint8_t* dst) {
	int remaining = this->dataSize - this->readCursor;
	if (remaining <= 0) {
		if (!this->looping) {
			this->eof = true;
			return 0;
		}
		this->readCursor = 0;
		remaining = this->dataSize;
	}
	int n = std::min(CHUNK_BYTES, remaining);
	if (!this->stream->readChunk(dst, this->dataOffset + this->readCursor, n)) {
		return 0;
	}
	this->readCursor += n;
	return n;
}

int Sound::MusicStream::readNextOggChunk(uint8_t* dst) {
#if defined(DRPG_HAS_VORBIS)
	int totalRead = 0;
	int section = 0;
	while (totalRead < CHUNK_BYTES) {
		long ret = ov_read(&this->ogg->vf, (char*)dst + totalRead,
		                   CHUNK_BYTES - totalRead, 0, 2, 1, &section);
		if (ret == 0) {
			if (this->looping) {
				ov_pcm_seek(&this->ogg->vf, 0);
				continue;
			}
			this->eof = true;
			break;
		}
		if (ret < 0) {
			// Decode error; bail out of this chunk.
			break;
		}
		totalRead += (int)ret;
	}
	return totalRead;
#else
	(void)dst;
	return 0;
#endif
}

int Sound::MusicStream::readNextChunk(uint8_t* dst) {
	return this->isOgg ? this->readNextOggChunk(dst) : this->readNextWavChunk(dst);
}

bool Sound::MusicStream::start(const char* fileName, ALuint source, bool loop) {
	this->stream = std::make_unique<InputStream>();
	if (!this->stream->openForStreaming(fileName, LT_SOUND_RESOURCE)) {
		LOG_WARN("[sound] MusicStream: openForStreaming failed for {}\n", fileName);
		this->stream.reset();
		return false;
	}

	// Sniff first 4 bytes to choose between WAV (RIFF) and Ogg (OggS).
	uint8_t magic[4] = {};
	if (!this->stream->readChunk(magic, 0, 4)) {
		LOG_WARN("[sound] MusicStream: cannot read magic bytes from {}\n", fileName);
		this->stream.reset();
		return false;
	}

	if (std::memcmp(magic, "OggS", 4) == 0) {
#if defined(DRPG_HAS_VORBIS)
		this->ogg = std::make_unique<OggDecoder>();
		this->ogg->stream = this->stream.get();
		this->ogg->cursor = 0;
		if (ov_open_callbacks(this->ogg.get(), &this->ogg->vf, nullptr, 0, ovCallbacks) != 0) {
			LOG_WARN("[sound] MusicStream: ov_open_callbacks failed for {}\n", fileName);
			this->ogg.reset();
			this->stream.reset();
			return false;
		}
		this->ogg->opened = true;
		vorbis_info* info = ov_info(&this->ogg->vf, -1);
		if (!info) {
			LOG_WARN("[sound] MusicStream: ov_info returned null for {}\n", fileName);
			this->ogg.reset();
			this->stream.reset();
			return false;
		}
		this->sampleRate = (int)info->rate;
		if (info->channels == 1) this->format = AL_FORMAT_MONO16;
		else if (info->channels == 2) this->format = AL_FORMAT_STEREO16;
		else {
			LOG_WARN("[sound] MusicStream: unsupported channel count {} in {}\n",
			         info->channels, fileName);
			this->ogg.reset();
			this->stream.reset();
			return false;
		}
		this->isOgg = true;
		this->dataOffset = 0;
		this->dataSize = 0;
#else
		LOG_WARN("[sound] MusicStream: Ogg file {} but engine built without DRPG_HAS_VORBIS\n", fileName);
		this->stream.reset();
		return false;
#endif
	} else if (std::memcmp(magic, "RIFF", 4) == 0) {
		if (!parseWavHeader(this->stream.get(), &this->format, &this->sampleRate,
		                    &this->dataOffset, &this->dataSize)) {
			LOG_WARN("[sound] MusicStream: parseWavHeader failed for {}\n", fileName);
			this->stream.reset();
			return false;
		}
		this->isOgg = false;
	} else {
		LOG_WARN("[sound] MusicStream: unknown magic in {} ({:02x}{:02x}{:02x}{:02x})\n",
		         fileName, magic[0], magic[1], magic[2], magic[3]);
		this->stream.reset();
		return false;
	}

	this->readCursor = 0;
	this->eof = false;
	this->looping = loop;

	alGenBuffers(NUM_BUFFERS, this->streamBuffers);

	uint8_t chunk[CHUNK_BYTES];
	int queued = 0;
	for (int i = 0; i < NUM_BUFFERS; ++i) {
		int n = this->readNextChunk(chunk);
		if (n <= 0) break;
		alBufferData(this->streamBuffers[i], this->format, chunk, n, this->sampleRate);
		alSourceQueueBuffers(source, 1, &this->streamBuffers[i]);
		++queued;
	}
	if (queued == 0) {
		LOG_WARN("[sound] MusicStream: empty data for {}\n", fileName);
		alDeleteBuffers(NUM_BUFFERS, this->streamBuffers);
		std::memset(this->streamBuffers, 0, sizeof(this->streamBuffers));
		this->ogg.reset();
		this->stream.reset();
		return false;
	}

	alSourcei(source, AL_LOOPING, AL_FALSE); // we handle looping ourselves
	alSourcePlay(source);
	LOG_INFO("[sound] MusicStream: started {} ({}, {} Hz, loop={})\n",
	         fileName, this->isOgg ? "ogg" : "wav", this->sampleRate, loop ? 1 : 0);
	return true;
}

void Sound::MusicStream::update(ALuint source) {
	ALint processed = 0;
	alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

	uint8_t chunk[CHUNK_BYTES];
	while (processed-- > 0) {
		ALuint buf = 0;
		alSourceUnqueueBuffers(source, 1, &buf);
		if (buf == 0) break;
		int n = this->readNextChunk(chunk);
		if (n > 0) {
			alBufferData(buf, this->format, chunk, n, this->sampleRate);
			alSourceQueueBuffers(source, 1, &buf);
		}
		// else: source will drain naturally; we don't requeue at EOF.
	}

	// If the source stopped because all queued buffers drained, but we still
	// have data to play (looping turned on mid-track or we just barely missed
	// the refill window), kick it back into play.
	ALint state = AL_STOPPED;
	alGetSourcei(source, AL_SOURCE_STATE, &state);
	if (state != AL_PLAYING && state != AL_PAUSED && !this->eof) {
		ALint queued = 0;
		alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);
		if (queued > 0) {
			alSourcePlay(source);
		}
	}
}

void Sound::MusicStream::stop(ALuint source) {
	alSourceStop(source);
	alSourcei(source, AL_BUFFER, 0); // detaches all queued/processed buffers
	for (ALuint& buf : this->streamBuffers) {
		if (buf != 0) {
			alDeleteBuffers(1, &buf);
			buf = 0;
		}
	}
	this->ogg.reset();    // ov_clear() in ~OggDecoder
	this->stream.reset();
}

// ---------------------------------------------------------------------------
// Sound class
// ---------------------------------------------------------------------------

Sound::Sound() = default;

Sound::~Sound() {
	this->assetLoader.shutdown();
	if (this->headless) { return; }
	this->soundStop();
	this->openAL_Close();
}

#define OpenAL_ERROR(id)                                                                                               \
	error = alGetError();                                                                                              \
	if (error != AL_NO_ERROR) {                                                                                        \
		LOG_ERROR("OpenAL error: {}, file: {} [{}]\n", alGetString(error),                                                \
		       "/Users/greghodges/doom2rpg/trunk/Doom2rpg_iphone/xcode/Classes/Sound.cpp", id);                        \
	}

bool Sound::startup() {
	this->app = CAppContainer::getInstance()->app;
	this->headless = CAppContainer::getInstance()->headless;
	LOG_INFO("[sound] startup\n");

	this->hasDeferredSound = 0;
	this->app = app;
	this->allowSounds = true;
	this->allowMusics = true;
	this->soundFxVolume = 80;
	this->musicVolume = 80;
	this->soundCapacity = 100;
	this->isMuted = 0;
	this->unused_0x160 = -1;
	this->soundsLoaded = false;
	this->alContext = nullptr;

	// sounds.yaml is already loaded in loadTables() (before menuSystem needs it)

	this->channel.clear();
	this->channel.reserve(8); // small initial reserve; channels are allocated on demand

	if (!this->headless) {
		this->openAL_Init();
	}

	return true;
}

void Sound::openAL_Init() {
	ALenum error;
	LOG_INFO("[sound] openAL_Init\n");

	alGetError();
	if (alcGetCurrentContext()) {
		std::println(stderr, "WARNING! Trying to init OpenAL, but we already have a context!");
	} else {
		this->alDevice = alcOpenDevice(nullptr);
		OpenAL_ERROR(522);

		if (this->alDevice) {
			ALCcontext* context = alcCreateContext(this->alDevice, nullptr);
			OpenAL_ERROR(527);

			if (context) {
				alcMakeContextCurrent(context);
				this->alContext = context;
			} else {
				alcCloseDevice(this->alDevice);
			}
		}
		OpenAL_ERROR(540);

		// this->openAL_SetSystemVolume(100);
		alListener3i(AL_POSITION, 0, 0, 0);
		alListener3i(AL_VELOCITY, 0, 0, 0);
		OpenAL_ERROR(546);
	}
}

void Sound::openAL_Close() {
	// Drop the preloaded SFX cache before deleting buffers.
	for (auto& kv : this->preloadedBuffers) {
		if (kv.second != 0) { alDeleteBuffers(1, &kv.second); }
	}
	this->preloadedBuffers.clear();

	for (auto& ch : this->channel) {
		if (ch.bufferId != 0) { alDeleteBuffers(1, &ch.bufferId); }
		if (ch.sourceId != 0) { alDeleteSources(1, &ch.sourceId); }
	}
	this->channel.clear();

	alcMakeContextCurrent(nullptr);
	if (this->alContext) {
		alcDestroyContext(this->alContext);
		this->alContext = nullptr;
	}
	if (this->alDevice) {
		alcCloseDevice(this->alDevice);
		this->alDevice = nullptr;
	}
}

void Sound::openAL_SetSystemVolume(int volume) {
	if (volume > 100) {
		volume = 100;
	}
	alListenerf(AL_GAIN, (float)volume / 100.0f);
}

void Sound::openAL_SetVolume(ALuint source, int volume) {
	if (volume > 100) {
		volume = 100;
	}
	alSourcef(source, AL_MAX_GAIN, (float)volume / 100.0);
}

void Sound::openAL_Suspend() {
	if (this->alContext) {
		alcMakeContextCurrent(nullptr);
		alcSuspendContext(this->alContext);
	}
}

void Sound::openAL_Resume() {
	if (this->alContext) {
		alcMakeContextCurrent(this->alContext);
		alcProcessContext(this->alContext);
	}
}

bool Sound::openAL_IsPlaying(ALuint source) {
	ALenum error;
	ALint source_state;
	alGetSourcei(source, AL_SOURCE_STATE, &source_state);
	OpenAL_ERROR(685);
	return (source_state == AL_PLAYING) ? true : false;
}

bool Sound::openAL_IsPaused(ALuint source) {
	ALenum error;
	ALint source_state;
	alGetSourcei(source, AL_SOURCE_STATE, &source_state);
	OpenAL_ERROR(694);
	return (source_state == AL_PAUSED) ? true : false;
}

bool Sound::openAL_GetALFormat(AudioStreamBasicDescription aStreamBD, ALenum* format) {
	if ((aStreamBD.mFormatID == 'lpcm') && (aStreamBD.mChannelsPerFrame - 1U < 2)) {
		if (aStreamBD.mBitsPerChannel == 8) {
			if (aStreamBD.mChannelsPerFrame == 1) {
				*format = AL_FORMAT_MONO8;
			} else {
				*format = AL_FORMAT_STEREO8;
			}
			return true;
		}
		if (aStreamBD.mBitsPerChannel == 16) {
			if (aStreamBD.mChannelsPerFrame == 1) {
				*format = AL_FORMAT_MONO16;
			} else {
				*format = AL_FORMAT_STEREO16;
			}
			return true;
		}
	}
	return false;
}

void Sound::openAL_PlaySound(ALuint source, ALint loop) {
	ALenum error;
	alSourcei(source, AL_LOOPING, (loop != 0) ? AL_TRUE : AL_FALSE);
	OpenAL_ERROR(708);
	alSourcePlay(source);
	OpenAL_ERROR(711);
}

void Sound::openAL_LoadSound(int resID, Sound::SoundStream* channel) {
	ALenum error;
	LOG_INFO("[sound] openAL_LoadSound: resID={} buffer={}\n", resID, channel->bufferId);
	int index = (uint16_t)(resID - 1000);
	OpenAL_ERROR(596);

	const char* fileName = Sounds::getFileName(index);
	if (!fileName) {
		this->app->Error("Sound index %d out of range\n!", index);
		return;
	}

	LOG_INFO("[sound] Loading sound: name={} resID={} buffer={}\n", fileName, resID, channel->bufferId);
	if (this->openAL_LoadWAVFromFile(channel->bufferId, fileName)) {
		OpenAL_ERROR(606);
	} else {
		this->app->Error("Sound resource not found\n!");
	}
}

bool Sound::openAL_LoadWAVFromFile(ALuint bufferId, const char* fileName) {
	ALenum error;
	ALsizei freq;
	std::vector<uint8_t> data;
	ALenum format;

	LOG_INFO("[sound] Loading wav: {}\n", fileName);
	if (this->openAL_LoadAudioFileData(fileName, &format, data, &freq)) {
		alBufferData(bufferId, format, data.data(), (ALsizei)data.size(), freq);
		OpenAL_ERROR(939);
		return true;
	}
	return false;
}

bool Sound::openAL_LoadAudioFileData(const char* fileName, ALenum* format, std::vector<uint8_t>& data,
                                     ALsizei* freq) {
	InputStream IS;
	AudioStreamBasicDescription outPropertyData;
	char buffer[4];

	if (this->openAL_OpenAudioFile(fileName, &IS)) {
		IS.read((uint8_t*)buffer, 0, 4);
		if (strncmp(buffer, "RIFF", 4) != 0) {
			this->app->Error("Not a valid WAV file, RIFF not found in header\n!");
		}

		IS.readInt(); // size of file

		IS.read((uint8_t*)buffer, 0, 4);
		if (strncmp(buffer, "WAVE", 4) != 0) {
			this->app->Error("Not a valid WAV file, WAVE not found in header\n!");
		}

		IS.read((uint8_t*)buffer, 0, 4);
		if (strncmp(buffer, "fmt ", 4) != 0) {
			this->app->Error("Not a valid WAV file, Format Marker not found in header\n!");
		}

		int chunklength = IS.readInt();
		if ((chunklength != 16) && (chunklength != 18)) {
			this->app->Error("Not a valid WAV file, format length wrong in header\n!");
		}

		if (IS.readShort() != 1) {
			this->app->Error("Not a valid WAV file, file not in PCM format\n!");
		}

		outPropertyData.mFormatID = 'lpcm';
		outPropertyData.mChannelsPerFrame = IS.readShort(); // Get number of channels.
		outPropertyData.mSampleRate = IS.readInt();         // Get sampler rate.
		IS.readInt();                                       // Block Align
		IS.readShort();                                     // ByteRate
		outPropertyData.mBitsPerChannel = IS.readShort();   // Get Bits Per Sample.
		if (chunklength == 18) {                            // SpiderMastermind_sight.wav use chunklength 18
			IS.readShort();
		}
		IS.readInt();             // "data" chunk.
		int dataSize = IS.readInt();

		data.resize((size_t)dataSize);
		IS.read(data.data(), 0, dataSize);

		IS.close();
		IS.~InputStream();

		if (!this->openAL_GetALFormat(outPropertyData, format)) {
			this->app->Error("openAL: Error formatting");
		}

		*freq = (int)outPropertyData.mSampleRate;
		return true;
	}
	IS.~InputStream();
	return false;
}

bool Sound::openAL_OpenAudioFile(const char* fileName, InputStream* IS) {
	if (!IS->loadFile(fileName, LT_SOUND_RESOURCE)) {
		this->app->Error("Failed to open audio: %s", fileName);
		return false;
	}

	return true;
}

bool Sound::openAL_LoadAllSounds() {
	if (!this->soundsLoaded) {
		// Channels are now allocated on demand by allocateChannel(); nothing to pre-create.
		this->soundsLoaded = true;
		return true;
	}

	return false;
}

int Sound::allocateChannel() {
	ALenum error;
	this->channel.emplace_back();
	SoundStream& s = this->channel.back();
	s.resID = -1;
	s.priority = 1;
	alGenBuffers(1, &s.bufferId);
	alGenSources(1, &s.sourceId);
	alSource3i(s.sourceId, AL_POSITION, 0, 0, 0);
	alSourcei(s.sourceId, AL_REFERENCE_DISTANCE, 5000000);
	OpenAL_ERROR(643);
	return (int)this->channel.size() - 1;
}

bool Sound::cacheSounds() {
	if (this->headless) { return true; }
	if (this->openAL_LoadAllSounds()) {
		this->updateVolume();
		this->preloadAllSFX();
		return true;
	}
	return false;
}

void Sound::preloadAllSFX() {
	if (this->headless) { return; }
	this->assetLoader.start(); // idempotent
	int enqueued = 0;
	for (int i = 0; i < Sounds::getCount(); ++i) {
		// Skip music tracks — they stream via MusicStream and shouldn't be
		// pre-loaded into a single AL buffer.
		if (i >= 67 && i <= 71) continue;
		const char* fileName = Sounds::getFileName(i);
		if (!fileName || !fileName[0]) continue;
		this->assetLoader.enqueueSoundLoad(i + 1000, std::string(fileName));
		++enqueued;
	}
	LOG_INFO("[sound] preloadAllSFX: enqueued {} SFX for async load\n", enqueued);
}

void Sound::drainPreloadResults() {
	if (this->headless) { return; }
	AssetLoader::Result r;
	int published = 0;
	while (this->assetLoader.tryPopResult(r)) {
		if (!r.success || r.bytes.empty()) continue;
		// Already cached (e.g. lazy-loaded before the worker got to it).
		if (this->preloadedBuffers.count(r.resID)) continue;

		// Wrap the bytes in an in-memory InputStream and parse the WAV header.
		InputStream is;
		if (!is.loadFromBuffer(r.bytes.data(), (int)r.bytes.size())) continue;
		ALenum format = 0;
		int sampleRate = 0, dataOffset = 0, dataSize = 0;
		if (!parseWavHeader(&is, &format, &sampleRate, &dataOffset, &dataSize)) continue;
		if (dataOffset + dataSize > (int)r.bytes.size()) continue;

		ALuint buf = 0;
		alGenBuffers(1, &buf);
		if (buf == 0) continue;
		alBufferData(buf, format, r.bytes.data() + dataOffset, dataSize, sampleRate);
		this->preloadedBuffers[r.resID] = buf;
		++published;
	}
	if (published > 0) {
		LOG_DEBUG("[sound] drainPreloadResults: published {} buffers (cache size now {})\n",
		          published, (int)this->preloadedBuffers.size());
	}
}

void Sound::playSound(int16_t resID, uint8_t flags, int priority, bool unused) {
	if (this->headless) { return; }
	ALenum error;

	int soundResID;
	bool isInvalid;
	bool isBlocked;
	int existingSlot;
	int chanIdx;
	int stopIdx;
	int volume;
	int freeSlot;

	SoundStream* channel;

	soundResID = resID;
	isInvalid = resID == -1;
	if (resID != -1)
		isInvalid = resID == 255;

	if (!isInvalid) {
		isBlocked = resID == 1255;
		if (resID != 1255)
			isBlocked = this->soundCapacity == 0;
		if (!isBlocked && !this->isMuted) {
			if ((unsigned int)(resID - 1067) <= 4) {
				if (!this->allowMusics /* || isUserMusicOn()*/)
					return;
			} else if (!this->allowSounds) {
				return;
			}
			if (this->resID == soundResID)
				return;
			freeSlot = -1;
			for (chanIdx = 0; chanIdx < (int)this->channel.size(); chanIdx++) {
				if (this->channel[chanIdx].resID == soundResID) {
					if (!this->openAL_IsPlaying(this->channel[chanIdx].sourceId)) {
						existingSlot = chanIdx;
						goto check_play_mode;
					}
					if (priority == 6) {
						existingSlot = chanIdx;
						return;
					}
				}

				if (freeSlot == -1 && this->channel[chanIdx].resID == -1) {
					freeSlot = chanIdx;
				}
			}

			existingSlot = -1;
		check_play_mode:
			if (priority == 6 || (flags & 1) != 0) {
			skip_if_playing:
				if (existingSlot != -1 && this->openAL_IsPlaying(this->channel[existingSlot].sourceId))
					return;
			}
			if ((flags & 2) != 0) {
				for (stopIdx = 0; stopIdx < (int)this->channel.size(); stopIdx++) {
					alSourceStop(this->channel[stopIdx].sourceId);
					this->channel[stopIdx].priority = 1;
					this->channel[stopIdx].fadeInProgress = 0;
				}
			}

			if (existingSlot == -1) {
				if (freeSlot == -1) {
					freeSlot = this->getFreeSlot(priority);
					if (freeSlot == -1)
						return;
				}
				channel = &this->channel[freeSlot];
				// Tear down any prior streaming state on this slot before reuse.
				if (channel->music) {
					channel->music->stop(channel->sourceId);
					channel->music.reset();
				}
				channel->resID = resID;
				channel->priority = priority;
				alSourceStop(channel->sourceId);
				alSourcei(channel->sourceId, AL_BUFFER, 0);

				// Music tracks (resID 1067..1071) play via MusicStream — keeps the
				// file open and feeds OpenAL via 4 small queued buffers refilled
				// in startFrame(), instead of loading the entire 9–11 MB WAV.
				if ((unsigned int)(resID - 1067) <= 4) {
					int index = (uint16_t)(resID - 1000);
					const char* fileName = Sounds::getFileName(index);
					if (fileName) {
						auto stream = std::make_unique<MusicStream>();
						if (stream->start(fileName, channel->sourceId, (flags & 1) != 0)) {
							channel->music = std::move(stream);
							this->openAL_SetVolume(channel->sourceId, this->musicVolume);
							OpenAL_ERROR(258);
							return; // alSourcePlay was issued inside MusicStream::start
						}
						LOG_WARN("[sound] MusicStream failed for {}; falling back to full load\n", fileName);
					}
					// Fall through to non-streaming path on stream failure.
				}

				// Use the preloaded buffer if the AssetLoader has already published
				// one for this sound. Skips the per-play disk I/O + RIFF parse.
				auto preIt = this->preloadedBuffers.find(soundResID);
				if (preIt != this->preloadedBuffers.end()) {
					alSourcei(channel->sourceId, AL_BUFFER, preIt->second);
				} else {
					this->openAL_LoadSound(soundResID, &this->channel[freeSlot]);
					alSourcei(channel->sourceId, AL_BUFFER, channel->bufferId);
				}
				if ((unsigned int)(channel->resID - 1067) > 4)
					volume = this->soundFxVolume;
				else
					volume = this->musicVolume;
				this->openAL_SetVolume(channel->sourceId, volume);
				OpenAL_ERROR(258);
			} else {
				channel = &this->channel[existingSlot];
				if ((unsigned int)(channel->resID - 1067) > 4)
					volume = this->soundFxVolume;
				else
					volume = this->musicVolume;
				this->openAL_SetVolume(channel->sourceId, volume);
				OpenAL_ERROR(214);
			}

			this->openAL_PlaySound(channel->sourceId, (flags & 1) ? 1 : 0);
		}
	}
}

int Sound::getFreeSlot(int minPriority) {
	// Reuse a slot that's free (resID == -1) or whose source has stopped playing.
	for (size_t i = 0; i < this->channel.size(); ++i) {
		if (this->channel[i].resID == -1) {
			return (int)i;
		}
		ALint state;
		alGetSourcei(this->channel[i].sourceId, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING && state != AL_PAUSED) {
			return (int)i;
		}
	}
	// All slots are busy. Grow the pool if we're under the soft cap.
	if (this->channel.size() < MAX_CHANNELS) {
		return this->allocateChannel();
	}
	// At the cap — preempt the lowest-priority slot strictly below minPriority.
	int bestSlot = -1;
	int lowestPriority = minPriority;
	for (size_t i = 0; i < this->channel.size(); ++i) {
		if (this->channel[i].priority < lowestPriority) {
			bestSlot = (int)i;
			lowestPriority = this->channel[i].priority;
		}
	}
	return bestSlot;
}

void Sound::soundStop() {
	if (this->headless) { return; }
	for (size_t i = 0; i < this->channel.size(); ++i) {
		if (this->channel[i].music) {
			this->channel[i].music->stop(this->channel[i].sourceId);
			this->channel[i].music.reset();
		} else {
			alSourceStop(this->channel[i].sourceId);
		}
		this->channel[i].priority = 1;
		this->channel[i].fadeInProgress = false;
	}
}

void Sound::stopSound(int resID, bool fadeOut) {
	if (this->headless) { return; }
	int volume;
	for (size_t i = 0; i < this->channel.size(); ++i) {
		if (this->channel[i].resID == resID) {
			if (fadeOut) {
				if (!this->channel[i].fadeInProgress) {
					if ((unsigned int)((int16_t)resID - 1067) > 4)
						volume = this->soundFxVolume;
					else
						volume = this->musicVolume;
					this->channel[i].StartFade(volume, 0, 500);
				}
			} else {
				if (this->channel[i].music) {
					this->channel[i].music->stop(this->channel[i].sourceId);
					this->channel[i].music.reset();
				} else {
					alSourceStop(this->channel[i].sourceId);
				}
				this->channel[i].priority = 1;
				this->channel[i].fadeInProgress = false;
			}
			return;
		}
	}
}

bool Sound::isSoundPlaying(int16_t resID) {
	if (this->headless) { return false; }
	for (size_t i = 0; i < this->channel.size(); ++i) {
		if (this->channel[i].resID == resID) {
			return this->openAL_IsPlaying(this->channel[i].sourceId);
		}
	}
	return false;
}

void Sound::updateVolume() {
	if (this->headless) { return; }
	int volume;
	if (this->soundsLoaded) {
		for (size_t i = 0; i < this->channel.size(); ++i) {
			if (!this->channel[i].fadeInProgress) {
				if ((unsigned int)(this->channel[i].resID - 1067) > 4) {
					volume = this->soundFxVolume;
				} else {
					volume = this->musicVolume;
				}
				this->openAL_SetVolume(this->channel[i].sourceId, volume);
			}
		}

		this->allowMusics = (this->musicVolume != 0) ? true : false;
		this->allowSounds = (this->soundFxVolume != 0) ? true : false;
	}
}

void Sound::playCombatSound(int16_t resID, uint8_t flags, int priority) {
	this->playSound(resID, flags, priority, false);
}

bool Sound::cacheCombatSound(int resID) {
	// no implementado
	return true;
}
void Sound::freeMonsterSounds() {
	// no implementado
}

void Sound::volumeUp(int volume) {
	this->soundFxVolume += volume;
	if (this->soundFxVolume > 100) {
		this->soundFxVolume = 100;
	}
	this->updateVolume();
}

void Sound::volumeDown(int volume) {
	this->soundFxVolume -= volume;
	if (this->soundFxVolume < 0) {
		this->soundFxVolume = 0;
	}
	this->updateVolume();
}

void Sound::startFrame() {
	if (this->hasDeferredSound != 0) {
		if (!this->isMuted) {
			this->playSound(this->resID, this->flags, this->priority, this->isMuted);
			this->unused_0x160 = -1;
			this->resID = -1;
			this->flags = 0;
			this->priority = 0;
			this->unused_0x15c = -1;
			this->hasDeferredSound = 0;
		}
	}

	// Refill streaming buffers for any music slots. Cheap when there is no
	// music playing (just an empty range) — only ever 1–2 slots have streams.
	if (!this->headless) {
		for (size_t i = 0; i < this->channel.size(); ++i) {
			if (this->channel[i].music) {
				this->channel[i].music->update(this->channel[i].sourceId);
			}
		}
		// Publish any SFX that the AssetLoader worker finished reading.
		this->drainPreloadResults();
	}
}

void Sound::endFrame() {
	// no implementado
}

void Sound::SoundStream::StartFade(int volume, int fadeBeg, int fadeEnd) {
	this->fadeBeg = fadeBeg;
	this->fadeInProgress = true;
	this->volume = volume;
	this->fadetime = CAppContainer::getInstance()->app->gameTime;
	this->fadeVolume = (float)(fadeBeg - volume) / (float)fadeEnd;
}

void Sound::musicVolumeUp(int volume) { // [GEC]
	this->musicVolume += volume;
	if (this->musicVolume > 100) {
		this->musicVolume = 100;
	}
	this->updateVolume();
}

void Sound::musicVolumeDown(int volume) { // [GEC]
	this->musicVolume -= volume;
	if (this->musicVolume < 0) {
		this->musicVolume = 0;
	}
	this->updateVolume();
}
