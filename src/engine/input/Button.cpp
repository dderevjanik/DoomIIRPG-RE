#include <stdexcept>
#include "Log.h"

#include "CAppContainer.h"
#include "App.h"
#include "Canvas.h"
#include "Button.h"
#include "Image.h"
#include "Graphics.h"
#include "Sound.h"


// ---------------
// GuiRect Class
// ---------------
void GuiRect::Set(int x, int y, int w, int h) {
	this->y = y;
	this->w = w;
	this->x = x;
	this->h = h;
}

bool GuiRect::ContainsPoint(int x, int y) {
	return ((x >= this->x) && (y >= this->y) && (x <= (this->x + this->w)) && (y <= (this->y + this->h)));
}

// ---------------
// fmButton Class
// ---------------

fmButton::fmButton(int buttonID, int x, int y, int w, int h, int soundResID) {
	this->buttonID = buttonID;
	this->normalIndex = -1;
	this->highlightIndex = -1;
	this->selectedIndex = -1;
	this->drawButton = true;
	this->soundResID = (short)soundResID;
	this->highlightRed = 0.0f;
	this->highlightGreen = 0.3f;
	this->highlightBlue = 0.9f;
	this->highlightAlpha = 0.9f;
	this->normalRed = 0.2f;
	this->normalGreen = 0.6f;
	this->normalBlue = 0.3f;
	this->normalAlpha = 0.2f;
	this->SetTouchArea(x, y, w, h);
}

fmButton::~fmButton() {
}

void fmButton::SetImage(Image* image, bool center) {
	this->ptrNormalImages = nullptr;
	this->imgNormal = image;
	this->normalIndex = -1;
	if (center) {
		this->centerX = (this->touchArea.w - image->width) / 2;
		this->centerY = (this->touchArea.h - image->height) / 2;
	}
}
void fmButton::SetImage(Image** ptrImages, int imgIndex, bool center) {
	this->imgNormal = nullptr;
	this->ptrNormalImages = ptrImages;
	this->normalIndex = imgIndex;
	if (center) {
		this->centerX = (this->touchArea.w - ptrImages[imgIndex]->width) / 2;
		this->centerY = (this->touchArea.h - ptrImages[imgIndex]->height) / 2;
	}
}

void fmButton::SetHighlightImage(Image* imgHighlight, bool center) {
	this->ptrHighlightImages = nullptr;
	this->imgHighlight = imgHighlight;
	this->highlightIndex = -1;
	if (center) {
		this->highlightCenterX = (this->touchArea.w - imgHighlight->width) / 2;
		this->highlightCenterY = (this->touchArea.h - imgHighlight->height) / 2;
	}
}

void fmButton::SetHighlightImage(Image** ptrImgsHighlight, int imgHighlightIndex, bool center) {
	this->imgHighlight = nullptr;
	this->ptrHighlightImages = ptrImgsHighlight;
	this->highlightIndex = imgHighlightIndex;
	if (center) {
		this->highlightCenterX = (this->touchArea.w - ptrImgsHighlight[imgHighlightIndex]->width) / 2;
		this->highlightCenterY = (this->touchArea.h - ptrImgsHighlight[imgHighlightIndex]->height) / 2;
	}
}

void fmButton::SetGraphic(int index)
{
	if (this->ptrNormalImages) {
		this->normalIndex = index;
		this->centerX = (this->touchArea.w - this->ptrNormalImages[index]->width) / 2;
		this->centerY = (this->touchArea.h - this->ptrNormalImages[index]->height) / 2;
	}

	if (this->ptrHighlightImages) {
		this->highlightIndex = index;
		this->highlightCenterX = (this->touchArea.w - this->ptrHighlightImages[index]->width) / 2;
		this->highlightCenterY = (this->touchArea.h - this->ptrHighlightImages[index]->height) / 2;
	}
}

void fmButton::SetTouchArea(int x, int y, int w, int h) {
#if 0 // Old
	this->touchArea.x = x;
	this->touchArea.y = y;
	this->touchArea.w = w;
	this->touchArea.h = h;
#endif
	this->SetTouchArea(x, y, w, h, true);
}

void fmButton::SetTouchArea(int x, int y, int w, int h, bool drawing) { // Port: new
	this->touchArea.x = x;
	this->touchArea.y = y;
	this->touchArea.w = w;
	this->touchArea.h = h;

	if (drawing) {
		this->touchAreaDrawing.x = x;
		this->touchAreaDrawing.y = y;
		this->touchAreaDrawing.w = w;
		this->touchAreaDrawing.h = h;
	}
}

void fmButton::SetHighlighted(bool highlighted) {
	if (!this->app) this->app = CAppContainer::getInstance()->app;
	Applet* app = this->app;

	if (this->drawButton != 0) {

		if (!this->highlighted && highlighted) {
			if (this->soundResID != -1) {
				if (!app->sound->isSoundPlaying(this->soundResID)) {
					app->sound->playSound(this->soundResID, 0, 5, false);
				}
			}
		}
		this->highlighted = highlighted;
	}
}

void fmButton::Render(Graphics* graphics) {
	if (!this->app) this->app = CAppContainer::getInstance()->app;
	Applet* app = this->app;

	//printf("fmButton::Render\n");

	if (!this->drawButton)
		return;

	if (this->highlighted) {
		if (this->imgHighlight){
			if (this->highlightRenderMode == 13) {
				app->canvas->setBlendSpecialAlpha((float)(app->canvas->m_controlAlpha * 0.01f));
			}
			graphics->drawImage(this->imgHighlight,
				this->touchAreaDrawing.x + this->highlightCenterX, this->touchAreaDrawing.y + this->highlightCenterY, 0, 0, this->highlightRenderMode);
			return;
		}
		else {
			if (!this->drawTouchArea && this->imgNormal) {
				graphics->drawImage(this->imgNormal,
					this->touchAreaDrawing.x + this->centerX, this->touchAreaDrawing.y + this->centerY, 0, 0, this->highlightRenderMode);
				return;
			}
			else if (this->ptrHighlightImages && this->highlightIndex != -1) {
				if (this->highlightRenderMode == 13) {
					app->canvas->setBlendSpecialAlpha((float)(app->canvas->m_controlAlpha * 0.01f));
				}
				graphics->drawImage(ptrHighlightImages[this->highlightIndex],
					this->touchAreaDrawing.x + this->highlightCenterX, this->touchAreaDrawing.y + this->highlightCenterY, 0, 0, this->highlightRenderMode);
				return;
			}
		}
	}

	if (this->imgNormal) {
		if (this->normalRenderMode == 13){
			app->canvas->setBlendSpecialAlpha((float)(app->canvas->m_controlAlpha * 0.01f));
		}
		graphics->drawImage(this->imgNormal,
			this->touchAreaDrawing.x + this->centerX, this->touchAreaDrawing.y + this->centerY, 0, 0, this->normalRenderMode);
		return;
	}

	if (!this->ptrNormalImages || (this->ptrNormalImages && this->normalIndex == -1))
	{
		if (this->drawTouchArea) {
			if (this->highlighted) {
				graphics->FMGL_fillRect(this->touchAreaDrawing.x, this->touchAreaDrawing.y, this->touchAreaDrawing.w, this->touchAreaDrawing.h,
					this->highlightRed, this->highlightGreen, this->highlightBlue, this->highlightAlpha);
			}
			else {
				graphics->FMGL_fillRect(this->touchAreaDrawing.x, this->touchAreaDrawing.y, this->touchAreaDrawing.w, this->touchAreaDrawing.h,
					this->normalRed, this->normalGreen, this->normalBlue, this->normalAlpha);
			}
		}
	}
	else {
		if (this->normalRenderMode == 13) {
			app->canvas->setBlendSpecialAlpha((float)(app->canvas->m_controlAlpha * 0.01f));
		}
		graphics->drawImage(this->ptrNormalImages[this->normalIndex],
			this->touchAreaDrawing.x + this->centerX, this->touchAreaDrawing.y + this->centerY, 0, 0, this->normalRenderMode);
	}
}

// ------------------------
// fmButtonContainer Class
// ------------------------

void createButtons(fmButtonContainer* container, std::span<const ButtonDef> defs) {
	for (const auto& def : defs) {
		fmButton* btn = new fmButton(def.id, def.x, def.y, def.w, def.h, def.soundResID);
		container->AddButton(btn);
	}
}

fmButtonContainer::fmButtonContainer() = default;

fmButtonContainer::~fmButtonContainer() {
	fmButton* next;
	for (fmButton* button = this->next; button != nullptr; button = next){
		next = button->next;
		delete button;
	}
}

void fmButtonContainer::AddButton(fmButton* button) {
	if (this->next == nullptr) {
		this->next = button;
	}
	else {
		this->prev->next = button;
	}
	this->prev = button;
}

fmButton* fmButtonContainer::GetButton(int buttonID) {
	for (fmButton* next = this->next; next != nullptr; next = next->next) {
		if (next->buttonID == buttonID) {
			return next;
		}
	}

	return nullptr;
}

fmButton* fmButtonContainer::GetTouchedButton(int x, int y) {
	for (fmButton* next = this->next; next != nullptr; next = next->next) {
		if (next->drawButton && 
			(x >= next->touchArea.x) && (y >= next->touchArea.y) &&
			(x <= (next->touchArea.x + next->touchArea.w)) &&
			(y <= (next->touchArea.y + next->touchArea.h))) {
			return next;
		}
	}
	return nullptr;
}

int fmButtonContainer::GetTouchedButtonID(int x, int y) {
	for (fmButton* next = this->next; next != nullptr; next = next->next) {
		if (next->drawButton && 
			(x >= next->touchArea.x) && (y >= next->touchArea.y) &&
			(x <= (next->touchArea.x + next->touchArea.w)) &&
			(y <= (next->touchArea.y + next->touchArea.h))) {
			return next->buttonID;
		}
	}
	return -1;
}

int fmButtonContainer::GetHighlightedButtonID() {
	for (fmButton* next = this->next; next != nullptr; next = next->next) {
		if (next->drawButton && next->highlighted) {
			return next->buttonID;
		}
	}
	return -1;
}

void fmButtonContainer::HighlightButton(int x, int y, bool highlighted) {
	if (highlighted) {
		for (fmButton* next = this->next; next != nullptr; next = next->next) {
			if ((x >= next->touchArea.x) && (y >= next->touchArea.y) &&
				(x <= (next->touchArea.x + next->touchArea.w)) &&
				(y <= (next->touchArea.y + next->touchArea.h))) {
				next->SetHighlighted(true);
			}
			else {
				next->SetHighlighted(false);
			}
		}
	}
	else {
		for (fmButton* next = this->next; next != nullptr; next = next->next) {
			next->SetHighlighted(false);
		}
	}
}

void fmButtonContainer::SetGraphic(int index) {
	for (fmButton* next = this->next; next != nullptr; next = next->next) {
		next->SetGraphic(index);
	}
}

void fmButtonContainer::FlipButtons() {
	for (fmButton* next = this->next; next != nullptr; next = next->next) {
		next->touchArea.x = -next->touchArea.x - next->touchArea.w + Applet::IOS_WIDTH;
		next->touchAreaDrawing.x = -next->touchAreaDrawing.x - next->touchAreaDrawing.w + Applet::IOS_WIDTH; // New
	}
}

void fmButtonContainer::Render(Graphics* graphics) {
	for (fmButton* next = this->next; next != nullptr; next = next->next) {
		next->Render(graphics);
	}
}

// ---------------------
// fmScrollButton Class
// ---------------------

fmScrollButton::fmScrollButton(int x, int y, int w, int h, bool b, int soundResID) {

	this->imgBar = nullptr;
	this->imgBarTop = nullptr;
	this->imgBarMiddle = nullptr;
	this->imgBarBottom = nullptr;
	this->barTouched = 0;
	this->isVertical = b;
	this->barRect.x = x;
	this->barRect.y = y;
	this->barRect.w = w;
	this->barRect.h = h;
	this->enabled = 0;
	this->boxRect.x = 0;
	this->boxRect.y = 0;
	this->boxRect.w = 0;
	this->boxRect.h = 0;
	this->contentDragging = 0;
	this->visibleSize = 0;
	this->contentSize = 0;
	this->scrollOffset = 0;
	this->thumbPosition = 0;
	this->thumbSize = 0;
	this->scrollRatio = 0;
	this->thumbDragOffset = 0;
	this->touchStartPos = 0;
	this->soundResID = (short)soundResID;
}

fmScrollButton::~fmScrollButton() {
}

void fmScrollButton::SetScrollBarImages(Image* imgBar, Image* imgBarTop, Image* imgBarMiddle, Image* imgBarBottom) {
	this->imgBar = imgBar;
	this->imgBarTop = imgBarTop;
	this->imgBarMiddle = imgBarMiddle;
	this->imgBarBottom = imgBarBottom;
}

void fmScrollButton::SetScrollBox(int x, int y, int w, int h, int totalContentSize) {
	int computedThumbSize;
	int barSize;
	int visSize;
	float contentRange;
	float thumbRange;

	this->visibleSize = w;
	(this->boxRect).w = w;
	barSize = (this->barRect).w;
	if (this->isVertical) {
		this->visibleSize = h;
		barSize = (this->barRect).h;
	}
	visSize = this->visibleSize;
	(this->boxRect).y = y;
	(this->boxRect).x = x;
	(this->boxRect).h = h;
	this->contentSize = totalContentSize;
	this->scrollOffset = 0;
	computedThumbSize = (visSize * barSize/ this->contentSize);
	this->thumbPosition = 0;
	contentRange = (float)(this->contentSize - visSize);
	this->thumbSize = computedThumbSize;
	thumbRange = (float)(barSize - computedThumbSize);
	this->scrollRatio = (contentRange / thumbRange);
}

void fmScrollButton::SetScrollBox(int x, int y, int w, int h, int totalContentSize, int thumbSizeOverride)
{
	int contentTotal;
	int visSize;
	int barSize;
	bool vertical;
	float contentRange;
	float thumbRange;

	this->visibleSize = w;
	(this->boxRect).w = w;
	vertical = this->isVertical;
	(this->boxRect).x = x;
	(this->boxRect).y = y;
	vertical = vertical;
	this->contentSize = totalContentSize;
	this->scrollOffset = 0;
	if (vertical) {
		this->visibleSize = h;
	}
	this->thumbPosition = 0;
	contentTotal = this->contentSize;
	visSize = this->visibleSize;
	barSize = (this->barRect).w;
	if (vertical) {
		barSize = (this->barRect).h;
	}
	(this->boxRect).h = h;
	contentRange = (float)(contentTotal - visSize);
	this->thumbSize = thumbSizeOverride;
	thumbRange = (float)(barSize - thumbSizeOverride);
	this->scrollRatio = contentRange / thumbRange;
	return;
}

void fmScrollButton::SetContentTouchOffset(int x, int y)
{
	if (this->enabled == 0) {
		return;
	}
	if (this->isVertical) {
		x = y;
	}
	this->touchStartPos = x;
	this->touchStartScrollOffset = this->scrollOffset;
	return;
}

void fmScrollButton::UpdateContent(int x, int y)
{
	int newOffset;
	int maxOffset;
	float offsetFloat;

	if (this->enabled != 0) {
		if (!this->isVertical) {
			newOffset = (this->touchStartScrollOffset - x) + this->touchStartPos;
		}
		else {
			newOffset = (this->touchStartScrollOffset - y) + this->touchStartPos;
		}
		this->scrollOffset = newOffset;
		newOffset = this->scrollOffset;
		maxOffset = this->contentSize - this->visibleSize;
		if (newOffset < 0) {
			newOffset = 0;
			this->scrollOffset = 0;
		}
		if (maxOffset < newOffset) {
			this->scrollOffset = maxOffset;
			newOffset = maxOffset;
		}
		offsetFloat = (float)newOffset;
		this->thumbPosition = (int)(offsetFloat / this->scrollRatio);
	}
}

void fmScrollButton::SetTouchOffset(int x, int y)
{
	if (this->enabled != 0) {
		if (!this->isVertical) {
			this->thumbDragOffset = (x - (this->barRect).x) - this->thumbPosition;
		}
		else {
			this->thumbDragOffset = (y - (this->barRect).y) - this->thumbPosition;
		}
	}
}

void fmScrollButton::Update(int x, int y) {
	Applet* app = CAppContainer::getInstance()->app;
	int barHeight;
	int newThumbPos;
	int dragOffset;
	int in_cr7;
	int clampedOffset;

	if (this->enabled != 0) {
		dragOffset = this->thumbDragOffset;
		barHeight = (this->barRect).h;
		newThumbPos = ((y - (this->barRect).y) - (this->thumbSize >> 1)) - dragOffset;
		if (newThumbPos < 0) {
			clampedOffset = 0;
		}
		this->thumbPosition = newThumbPos;
		if (newThumbPos < 0) {
			this->thumbPosition = clampedOffset;
		}
		else {
			barHeight = barHeight - this->thumbSize;
			if (newThumbPos <= barHeight) {
				this->scrollOffset = (int)((float)(newThumbPos) * this->scrollRatio);
				if (this->soundResID == -1) {
					return;
				}
				if (!app->sound->isSoundPlaying(this->soundResID)) {
					app->sound->playSound(this->soundResID, 0, 5, false);
				}
				return;
			}
			this->thumbPosition = barHeight;
			clampedOffset = this->contentSize - this->visibleSize;
		}
		this->scrollOffset = clampedOffset;
	}
	return;
}

void fmScrollButton::Render(Graphics* graphics) {
	//printf("fmScrollButton::Render\n");

	Image* img;
	int thumbColor;
	int drawX;
	int drawY;
	int drawW;
	int centeredX;

	if (this->enabled) {
		img = this->imgBar;
		if (img == (Image*)0x0) {
			graphics->setColor(0x7f303030);
			graphics->fillRect(this->barRect.x, this->barRect.y, this->barRect.w, this->barRect.h);
			if (this->barTouched == 0) {
				thumbColor = -0x555556;
			}
			else {
				thumbColor = -0x111156;
			}
			graphics->setColor(thumbColor);
			if (!this->isVertical) {
				drawW = (this->barRect).h;
				drawY = (this->barRect).y;
				drawX = this->thumbPosition + (this->barRect).x;
				thumbColor = this->thumbSize;
			}
			else {
				drawW = this->thumbSize;
				drawX = (this->barRect).x;
				thumbColor = (this->barRect).w;
				drawY = this->thumbPosition + (this->barRect).y;
			}
			graphics->fillRect(drawX, drawY, thumbColor, drawW);
		}
		else {
			thumbColor = this->barRect.w;
			drawX = this->barRect.x;
			centeredX = drawX + (thumbColor - this->imgBarTop->width >> 1);
			graphics->drawImage(img, drawX + (thumbColor - img->width >> 1), (this->barRect).y, 0, 0, 0);
			graphics->drawImage(this->imgBarTop, centeredX, (this->barRect).y + this->thumbPosition, 0, 0, 0);
			thumbColor = (this->barRect).y + this->thumbPosition;
			drawX = this->thumbSize;
			drawY = this->imgBarBottom->height;
			for (drawW = thumbColor + this->imgBarTop->height; drawW < (thumbColor + drawX) - drawY;
				drawW = drawW + this->imgBarMiddle->height) {
				graphics->drawImage(this->imgBarMiddle, centeredX, drawW, 0, 0, 0);
			}
			graphics->drawImage(this->imgBarBottom, centeredX, (this->barRect.y + this->thumbPosition + this->thumbSize) -
				this->imgBarBottom->height, 0, 0, 0);
		}
	}
}

// ------------------
// fmSwipeArea Class
// ------------------

fmSwipeArea::fmSwipeArea(int x, int y, int w, int h, int endX, int endY) {
	this->rect.x = x;
	this->rect.y = y;
	this->rect.w = w;
	this->rect.h = h;
	this->enable = true;
	this->begX = -1;
	this->begY = -1;
	this->curX = -1;
	this->curY = -1;
	this->endX = endX;
	this->endY = endY;
}

fmSwipeArea::~fmSwipeArea() {
}

int fmSwipeArea::UpdateSwipe(int x, int y, SwipeDir* swDir) {
	int iVar1;
	int iVar2;
	int iVar3;
	int iVar4;
	int iVar5;

	*swDir = SwipeDir::Null;
	if ((!this->touched) || (!this->enable)) {
		return 0;
	}
	iVar3 = this->curX;
	if ((iVar3 < 0) || (iVar1 = this->curY, iVar1 < 0)) {
		this->curX = x;
		this->curY = y;
		return 0;
	}
	iVar2 = this->endX;
	iVar5 = this->begY - iVar2;
	if (y <= iVar5) {
		iVar1 = 0;
	}
	if (iVar5 < y) {
		iVar5 = this->begY + iVar2;
		if ((y < iVar5) && (iVar3 <= x)) {
			iVar4 = this->begX;
			if (iVar3 - iVar4 <= this->endY) goto no_swipe;
			iVar1 = 1;
			this->touched = false;
			*swDir = SwipeDir::Right;
			iVar4 = this->begY;
			iVar2 = this->endX;
			iVar3 = iVar4 - iVar2;
			if (iVar3 < y) {
				iVar5 = iVar4 + iVar2;
			}
			if (iVar3 < y) {
				iVar4 = this->begX;
			}
			if (y <= iVar3) goto load_begX;
		}
		else {
			iVar4 = this->begX;
		no_swipe:
			iVar1 = 0;
		}
		if ((y < iVar5) && (this->endY < iVar4 - this->curX)) {
			this->touched = false;
			*swDir = SwipeDir::Left;
			iVar2 = this->endX;
			iVar1 = 1;
			goto load_begX;
		}
	}
	else {
	load_begX:
		iVar4 = this->begX;
	}
	if ((x <= iVar4 - iVar2) || (iVar4 = iVar4 + iVar2, iVar4 <= x)) goto store_and_return;
	if (this->endY < this->curY - this->begY) {
		this->touched = false;
		*swDir = SwipeDir::Down;
		if (this->begX - this->endX < x) {
			iVar1 = 1;
			iVar4 = this->begX + this->endX;
			goto check_up_swipe;
		}
	}
	else {
	check_up_swipe:
		if ((iVar4 <= x) || (this->begY - this->curY <= this->endY))
			goto store_and_return;
		this->touched = false;
		*swDir = SwipeDir::Up;
	}
	iVar1 = 1;
store_and_return:
	this->curX = x;
	this->curY = y;
	return iVar1;
}

void fmSwipeArea::Render(Graphics* graphics) {
	if (this->enable && this->drawTouchArea)
	{
		if (this->touched) {
			graphics->FMGL_fillRect(
				this->rect.x,// + 1,
				this->rect.y,// + 50,
				this->rect.w,// - 50,
				this->rect.h,// - 50,
				0.8f,
				0.8f,
				0.8f,
				0.7f);
		}
		else {
			graphics->FMGL_fillRect(
				this->rect.x,// + 1,
				this->rect.y,// + 50,
				this->rect.w,// - 50,
				this->rect.h,// - 50,
				0.5f,
				0.5f,
				0.5f,
				0.7f);
		}
	}
}
