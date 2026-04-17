#include <stdexcept>
#include <cstdlib>
#include <format>
#include <string>
#include "Log.h"

#include "SDLGL.h"
#include "CAppContainer.h"
#include "App.h"
#include "Render.h"
#include "GLES.h"
#include "ComicBook.h"
#include "Image.h"
#include "Graphics.h"
#include "Text.h"
#include "Sound.h"
#include "MenuSystem.h"
#include "Enums.h"
#include "Canvas.h"
#include "SoundNames.h"
#include "Sounds.h"

ComicBook::ComicBook() {
    this->exitBtnRect[0] = 10;
    this->exitBtnRect[1] = 10;
    this->exitBtnRect[2] = 60;
    this->exitBtnRect[3] = 30;
    this->unused_0x16c = 200;
    this->unused_0x170 = 20;
    this->unused_0x164 = 140;
    this->isLoaded = 0;
    this->comicBookIndex = 0;
    this->iPhoneComicIndex = 0;
    this->loadingCounter = 0;
    this->iPhonePage = 0;
    this->page = 0;
    this->curPage = 0;
    this->isTransitioning = 0;
    this->endPoint = 0;
    this->isSnappingBack = 0;
    this->unused_0x11c = 0;
    this->midPoint = 0;
    this->is_iPhoneComic = false;
    this->isFadingOut = 0;
    this->fadeCounter = 0;
    this->isOrientationSwitch = 0;
    this->transitionCounter = 0;
    this->drawExitButton = 0;
    this->frameCounter = 0;
    this->unused_0x168 = 280;
    this->exitCancelled = 0;
}

ComicBook::~ComicBook() {
}

int g_interfaceOrientation = 1; 
int resetOrientation;
bool GetOrientation(void)
{
    return g_interfaceOrientation != 4;
}
void SetOrientationRight(void)
{
    resetOrientation = 1;
}

void ComicBook::Draw(Graphics* graphics) {
    int comicBookIndex;
    int iPhoneComicIndex;
    bool canChangeOrientation;
    float alpha;
    int is_iPhoneComic;
    float fadeAlpha;
    bool isLandscape;
    bool isRotated;
    struct Image** imgiPhoneComicBook;
    int iPhonePage;
    char isFlipped;
    bool isPhone;
    int nextPage;
    int pageOffset;
    int maxPages;
    int prevPage;
    Image* nextImg;
    int nextPageIdx;
    Image** unused_imgArray;
    int pageStride;

    ++this->frameCounter;
    this->isLoaded = true;

    if (this->comicBookIndex <= 16) {
        if (!this->imgComicBook[this->comicBookIndex]) {
            this->loadImage(this->comicBookIndex, 1);
        }
        this->comicBookIndex++;
        this->isLoaded = false;
    }

    if (this->iPhoneComicIndex <= 38) {
        if (!this->imgiPhoneComicBook[this->iPhoneComicIndex]) {
            this->loadImage(this->iPhoneComicIndex, 0);
        }
        this->iPhoneComicIndex++;
        this->isLoaded = false;
    }

    if (!this->isLoaded) {
        this->DrawLoading(graphics);
        return;
    }

    if (this->is_iPhoneComic && !GetOrientation())
    {
        canChangeOrientation = !this->isOrientationSwitch;
        if (!this->isOrientationSwitch)
            canChangeOrientation = !this->isFadingOut;
        if (canChangeOrientation && !this->isTransitioning) {
            SetOrientationRight();
        }
    }

    this->UpdateMovement();
    this->UpdateTransition();
    app->render->_gles->ClearBuffer(0xFFFFFF);

    if (this->isOrientationSwitch)
    {
        alpha = (float)((float)this->transitionCounter / -30.0) + 1.0;
        fadeAlpha = 1.0 - alpha;
        if (this->is_iPhoneComic)
        {
            this->DrawImage(this->imgiPhoneComicBook[this->iPhonePage], 0, 0, 0, fadeAlpha, 0);
        }
        else
        {
            isLandscape = !GetOrientation();
            this->DrawImage(this->imgComicBook[this->page], 0, 0, !this->is_iPhoneComic, fadeAlpha, isLandscape);
        }
    }
    else
    {
        alpha = 1.0;
    }
    isRotated = this->is_iPhoneComic;
    imgiPhoneComicBook = this->imgiPhoneComicBook;
    iPhonePage = this->iPhonePage;
    if (isRotated)
    {
        iPhonePage = this->page;
        imgiPhoneComicBook = this->imgComicBook;
        if (!GetOrientation())
        {
            isFlipped = 1;
            isRotated = this->is_iPhoneComic;
            goto draw_current_page;
        }
        isRotated = this->is_iPhoneComic;
    }
    isFlipped = 0;
draw_current_page:
    this->DrawImage(imgiPhoneComicBook[iPhonePage], this->endPoint, 0, isRotated, alpha, isFlipped);
    if (this->isTouching || this->isTransitioning)
    {
        isPhone = this->is_iPhoneComic;
        nextPage = iPhonePage + 1;
        if (isPhone)
            pageOffset = -Applet::IOS_HEIGHT;
        else
            pageOffset = Applet::IOS_WIDTH;
        pageStride = pageOffset;
        if (this->is_iPhoneComic)
            maxPages = 17;
        else
            maxPages = 39;
        prevPage = maxPages - 1;
        if (nextPage > maxPages - 1)
            nextPage = 0;
        nextImg = imgiPhoneComicBook[nextPage];
        nextPageIdx = nextPage;
        if (!nextImg)
        {
            this->loadImage(nextPage, isPhone);
            isPhone = this->is_iPhoneComic;
            nextImg = imgiPhoneComicBook[nextPageIdx];
        }
        this->DrawImage(nextImg, pageStride + this->endPoint, 0, isPhone, alpha, isFlipped);
        if (iPhonePage - 1 >= 0) {
            prevPage = iPhonePage - 1;
        }
        if (!imgiPhoneComicBook[prevPage])
        {
            this->loadImage(prevPage, this->is_iPhoneComic);
        }
        this->DrawImage(imgiPhoneComicBook[prevPage], this->endPoint - pageStride, 0, this->is_iPhoneComic, alpha, isFlipped);
    }

    if (this->drawExitButton) {
        this->DrawExitButton(graphics);
    }
}

void ComicBook::DrawLoading(Graphics* graphics) {
    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;
    int x;
    int y;
    int flags;
    ++this->loadingCounter;
    app->render->_gles->ClearBuffer(0x000000);
    if (this->is_iPhoneComic) {
        x = 160;
        y = 240;
    }
    else {
        x = 240;
        y = 160;
    }
    Text* textBuff = app->localization->getSmallBuffer();
    textBuff->setLength(0);
    std::string loadingText;
    switch (this->frameCounter / 5 % 4)
    {
    case 0:
        loadingText = "Loading   ";
        break;
    case 1:
        loadingText = "Loading.  ";
        break;
    case 2:
        loadingText = "Loading.. ";
        break;
    case 3:
        loadingText = "Loading...";
        break;
    default:
        loadingText = "Loading....";
        break;
    }
    textBuff->append(loadingText.c_str());
    textBuff->dehyphenate();
    graphics->drawString(textBuff, x, y, (this->is_iPhoneComic) ? 67 : 3);
    textBuff->dispose();
}


void ComicBook::loadImage(int index, bool vComic) {
    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;
    if (vComic)
    {
        switch (index)
        {
        case 0:
            this->imgComicBook[0] = app->loadImage("comicbook/Cover.bmp", true);
            break;
        case 1:
            this->imgComicBook[1] = app->loadImage("comicbook/page_01.bmp", true);
            break;
        case 2:
            this->imgComicBook[2] = app->loadImage("comicbook/page_02.bmp", true);
            break;
        case 3:
            this->imgComicBook[3] = app->loadImage("comicbook/page_03.bmp", true);
            break;
        case 4:
            this->imgComicBook[4] = app->loadImage("comicbook/page_04.bmp", true);
            break;
        case 5:
            this->imgComicBook[5] = app->loadImage("comicbook/page_05.bmp", true);
            break;
        case 6:
            this->imgComicBook[6] = app->loadImage("comicbook/page_06.bmp", true);
            break;
        case 7:
            this->imgComicBook[7] = app->loadImage("comicbook/page_07.bmp", true);
            break;
        case 8:
            this->imgComicBook[8] = app->loadImage("comicbook/page_08.bmp", true);
            break;
        case 9:
            this->imgComicBook[9] = app->loadImage("comicbook/page_09.bmp", true);
            break;
        case 10:
            this->imgComicBook[10] = app->loadImage("comicbook/page_10.bmp", true);
            break;
        case 11:
            this->imgComicBook[11] = app->loadImage("comicbook/page_11.bmp", true);
            break;
        case 12:
            this->imgComicBook[12] = app->loadImage("comicbook/page_12.bmp", true);
            break;
        case 13:
            this->imgComicBook[13] = app->loadImage("comicbook/page_13.bmp", true);
        case 14:
            this->imgComicBook[14] = app->loadImage("comicbook/page_14.bmp", true);
            break;
        case 15:
            this->imgComicBook[15] = app->loadImage("comicbook/page_15.bmp", true);
            break;
        case 16:
            this->imgComicBook[16] = app->loadImage("comicbook/page_16.bmp", true);
            break;
        }
    }
    else
    {
        switch (index)
        {
        case 0:
            this->imgiPhoneComicBook[0] = app->loadImage("comicbook/frames/iPhone cover.bmp", true);
            break;
        case 1:
            this->imgiPhoneComicBook[1] = app->loadImage("comicbook/frames/iPhone page 1a.bmp", true);
            break;
        case 2:
            this->imgiPhoneComicBook[2] = app->loadImage("comicbook/frames/iPhone page 1b.bmp", true);
            break;
        case 3:
            this->imgiPhoneComicBook[3] = app->loadImage("comicbook/frames/iPhone page 2a.bmp", true);
            break;
        case 4:
            this->imgiPhoneComicBook[4] = app->loadImage("comicbook/frames/iPhone page 2b.bmp", true);
            break;
        case 5:
            this->imgiPhoneComicBook[5] = app->loadImage("comicbook/frames/iPhone page 2c.bmp", true);
            break;
        case 6:
            this->imgiPhoneComicBook[6] = app->loadImage("comicbook/frames/iPhone page 3a.bmp", true);
            break;
        case 7:
            this->imgiPhoneComicBook[7] = app->loadImage("comicbook/frames/iPhone page 3b.bmp", true);
            break;
        case 8:
            this->imgiPhoneComicBook[8] = app->loadImage("comicbook/frames/iPhone page 4a.bmp", true);
            break;
        case 9:
            this->imgiPhoneComicBook[9] = app->loadImage("comicbook/frames/iPhone page 4b.bmp", true);
            break;
        case 10:
            this->imgiPhoneComicBook[10] = app->loadImage("comicbook/frames/iPhone page 4c.bmp", true);
            break;
        case 11:
            this->imgiPhoneComicBook[11] = app->loadImage("comicbook/frames/iPhone page 5a.bmp", true);
            break;
        case 12:
            this->imgiPhoneComicBook[12] = app->loadImage("comicbook/frames/iPhone page 5b.bmp", true);
            break;
        case 13:
            this->imgiPhoneComicBook[13] = app->loadImage("comicbook/frames/iPhone page 5c.bmp", true);
            break;
        case 14:
            this->imgiPhoneComicBook[14] = app->loadImage("comicbook/frames/iPhone page 6a.bmp", true);
            break;
        case 15:
            this->imgiPhoneComicBook[15] = app->loadImage("comicbook/frames/iPhone page 6b.bmp", true);
            break;
        case 16:
            this->imgiPhoneComicBook[16] = app->loadImage("comicbook/frames/iPhone page 6c.bmp", true);
            break;
        case 17:
            this->imgiPhoneComicBook[17] = app->loadImage("comicbook/frames/iPhone page 7a.bmp", true);
            break;
        case 18:
            this->imgiPhoneComicBook[18] = app->loadImage("comicbook/frames/iPhone page 7b.bmp", true);
            break;
        case 19:
            this->imgiPhoneComicBook[19] = app->loadImage("comicbook/frames/iPhone page 8a.bmp", true);
            break;
        case 20:
            this->imgiPhoneComicBook[20] = app->loadImage("comicbook/frames/iPhone page 8b.bmp", true);
            break;
        case 21:
            this->imgiPhoneComicBook[21] = app->loadImage("comicbook/frames/iPhone page 8c.bmp", true);
            break;
        case 22:
            this->imgiPhoneComicBook[22] = app->loadImage("comicbook/frames/iPhone page 9.bmp", true);
            break;
        case 23:
            this->imgiPhoneComicBook[23] = app->loadImage("comicbook/frames/iPhone page 10a.bmp", true);
            break;
        case 24:
            this->imgiPhoneComicBook[24] = app->loadImage("comicbook/frames/iPhone page 10b.bmp", true);
            break;
        case 25:
            this->imgiPhoneComicBook[25] = app->loadImage("comicbook/frames/iPhone page 10c.bmp", true);
            break;
        case 26:
            this->imgiPhoneComicBook[26] = app->loadImage("comicbook/frames/iPhone page 11a.bmp", true);
            break;
        case 27:
            this->imgiPhoneComicBook[27] = app->loadImage("comicbook/frames/iPhone page 11b.bmp", true);
            break;
        case 28:
            this->imgiPhoneComicBook[28] = app->loadImage("comicbook/frames/iPhone page 12a.bmp", true);
            break;
        case 29:
            this->imgiPhoneComicBook[29] = app->loadImage("comicbook/frames/iPhone page 12b.bmp", true);
            break;
        case 30:
            this->imgiPhoneComicBook[30] = app->loadImage("comicbook/frames/iPhone page 13a.bmp", true);
            break;
        case 31:
            this->imgiPhoneComicBook[31] = app->loadImage("comicbook/frames/iPhone page 13b.bmp", true);
            break;
        case 32:
            this->imgiPhoneComicBook[32] = app->loadImage("comicbook/frames/iPhone page 14a.bmp", true);
            break;
        case 33:
            this->imgiPhoneComicBook[33] = app->loadImage("comicbook/frames/iPhone page 14b.bmp", true);
            break;
        case 34:
            this->imgiPhoneComicBook[34] = app->loadImage("comicbook/frames/iPhone page 15a.bmp", true);
            break;
        case 35:
            this->imgiPhoneComicBook[35] = app->loadImage("comicbook/frames/iPhone page 15b.bmp", true);
            break;
        case 36:
            this->imgiPhoneComicBook[36] = app->loadImage("comicbook/frames/iPhone page 15c.bmp", true);
            break;
        case 37:
            this->imgiPhoneComicBook[37] = app->loadImage("comicbook/frames/iPhone page 16a.bmp", true);
            break;
        case 38:
            this->imgiPhoneComicBook[38] = app->loadImage("comicbook/frames/iPhone page 16b.bmp", true);
            break;
        }
    }
}


void ComicBook::CheckImageExistence(Image* image) {
    uint8_t* pBmp;
    uint16_t* data;
    int texWidth, texHeight;

    if (!image->piDIB)
        return;

    if (image->texture == -1) {

        pBmp = image->piDIB->pBmp;
        image->texWidth = 1;
        image->texHeight = 1;

        while (texWidth = image->texWidth, texWidth < image->piDIB->width) {
            image->texWidth = texWidth << 1;
        }

        while (texHeight = image->texHeight, texHeight < image->piDIB->height) {
            image->texHeight = texHeight << 1;
        }

        data = (uint16_t*)malloc(sizeof(uint16_t) * texWidth * texHeight);
        image->isTransparentMask = false;
        for (int i = 0; i < image->piDIB->height; i++) {
            for (int j = 0; j < image->piDIB->width; j++) {
                int rgb = image->piDIB->pRGB565[pBmp[(image->piDIB->width * i) + j]];
                if (rgb == 0xF81F) {
                    image->isTransparentMask = true;
                }
                if (data != nullptr) {
                    data[(image->texWidth * i) + j] = rgb;
                }
            }
        }

        if (data) {
            image->CreateTexture(data, image->texWidth, image->texHeight);
            std::free(data);
        }
    }
}

void ComicBook::DrawImage(Image* image, int posX, int posY, char rotated, float alpha, char flipped)
{
    this->CheckImageExistence(image);
    int drawX = posX;
    int drawY = posY;
    if (rotated) {
        drawX = posY;
        drawY = posX;
    }
    image->DrawTextureAlpha(drawX, drawY, alpha, (bool)rotated, (bool)flipped);
}


void ComicBook::UpdateMovement() {
    int totalPages;
    bool is_iPhoneComic;
    int cur_iPhonePage;
    int iPhonePage;
    int curPage;
    int midPoint;
    int minSpeed;
    int speed;
    int endPoint;
    int newEndPoint;
    int lastPage;
    bool isWrapping;
    int screenSize;
    int snapBackDelta;

    if (this->isTouching)
        return;
    is_iPhoneComic = this->is_iPhoneComic;
    cur_iPhonePage = this->cur_iPhonePage;
    iPhonePage = this->iPhonePage;
    curPage = cur_iPhonePage;
    if (this->is_iPhoneComic)
    {
        iPhonePage = this->page;
        curPage = this->curPage;
    }
    else
    {
        totalPages = 39;
    }
    if (this->is_iPhoneComic)
        totalPages = 17;
    if (iPhonePage != curPage)
    {
        midPoint = this->midPoint;
        this->isTransitioning = 1;
        if (is_iPhoneComic)
            minSpeed = 5;
        else
            minSpeed = 8;
        speed = std::abs(midPoint);
        if (speed < minSpeed)
            speed = minSpeed;
        if (!is_iPhoneComic)
            speed = -speed;
        endPoint = this->endPoint;
        if (iPhonePage >= curPage)
            newEndPoint = endPoint - speed;
        else
            newEndPoint = speed + endPoint;
        this->endPoint = newEndPoint;
        if (iPhonePage)
        {
            lastPage = totalPages - 1;
        }
        else
        {
            lastPage = totalPages - 1;
            if (curPage == totalPages - 1)
            {
                newEndPoint -= 2 * speed;
                goto set_end_point;
            }
        }
        isWrapping = iPhonePage == lastPage;
        if (iPhonePage == lastPage)
            isWrapping = curPage == 0;
        if (!isWrapping)
            goto check_bounds;
        newEndPoint += 2 * speed;
    set_end_point:
        this->endPoint = newEndPoint;
    check_bounds:
        if (is_iPhoneComic)
            screenSize = Applet::IOS_HEIGHT;
        else
            screenSize = Applet::IOS_WIDTH;
        if (screenSize <= newEndPoint || newEndPoint <= -screenSize)
        {
            if (is_iPhoneComic)
                this->page = this->curPage;
            else
                this->iPhonePage = cur_iPhonePage;
            this->midPoint = 0;
            this->endPoint = 0;
            this->isTransitioning = 0;
        }
    }
    if (this->isSnappingBack)
    {
        this->isTransitioning = 1;
        snapBackDelta = 4 * this->endPoint / 5;
        this->endPoint = snapBackDelta;
        if (!snapBackDelta)
        {
            this->isSnappingBack = 0;
            this->isTransitioning = 0;
        }
    }
}

void ComicBook::UpdateTransition()
{
    bool isIdle;
    int newFadeCounter;
    bool fadeNotDone;
    bool is_iPhoneComic;
    float offsetStep;
    float currentOffset;
    int newTransCounter;
    float newOffset;

    isIdle = !this->isTransitioning;
    if (!this->isTransitioning)
    {
        isIdle = !this->isTouching;
    }
    if (isIdle)
    {
        if (this->isFadingOut)
        {
            newFadeCounter = this->fadeCounter + 1;
            fadeNotDone = this->fadeCounter - 29 < 0;
            this->fadeCounter = newFadeCounter;

            if (!(fadeNotDone ^ (newFadeCounter | 30) | (newFadeCounter == 30)))
            {
                this->fadeCounter = 0;
                this->isFadingOut = 0;
                this->transitionCounter = 0;
                this->isOrientationSwitch = 1;
            }
        }
        if (this->isOrientationSwitch)
        {
            is_iPhoneComic = this->is_iPhoneComic;
            offsetStep = -3.0;
            if (is_iPhoneComic)
                offsetStep = 3.0;
            currentOffset = this->transitionOffset;
            newTransCounter = this->transitionCounter + 1;
            this->transitionCounter = newTransCounter;
            newOffset = offsetStep + currentOffset;
            if (newTransCounter > 29)
            {
                this->is_iPhoneComic = !is_iPhoneComic;
                this->transitionCounter = 0;
                this->isOrientationSwitch = 0;
            }
            this->drawExitButton = false;
            this->transitionOffset = newOffset;
        }
    }
}


void ComicBook::Touch(int x, int y, bool b)
{
    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;
    bool isPressed;
    int touchY;
    bool wasCancelled;
    bool isTransitioning;
    bool is_iPhoneComic;
    int endPoint;
    int halfScreen;
    int midPoint;
    bool isPhoneComic;
    int page;
    int nextPageIdx;
    int prevPageIdx;
    int curPage;
    int wrappedPage;
    int cur_iPhonePage;
    int wrappedIPhonePage;

    isPressed = b;
    touchY = y;
    //printf("this->isLoaded %d\n", this->isLoaded);
    if (!this->isLoaded)
        return;

    this->curX = x;
    this->curY = y;
    if (this->ButtonTouch(x, y) && !app->sound->isSoundPlaying(1064)) {
        app->sound->playSound(Sounds::getResIDByName(SoundName::MENU_OPEN), 0, 5, 0);
    }

    if (isPressed)
    {
        this->frameCounter = 0;
    }
    else
    {
        if (this->exitBtnHighlighted)
        {
            wasCancelled = this->exitCancelled;
            this->exitBtnHighlighted = 0;
            if (!wasCancelled)
            {
                this->DeleteImages();
                app->menuSystem->back();
            }
        }
        this->exitCancelled = 0;
        //printf("this->frameCounter %d\n", this->frameCounter);
        if (this->frameCounter <= 8) { // Old 4
            this->drawExitButton ^= true;
        }
    }
    isTransitioning = this->isTransitioning;
    if (this->isTransitioning || !this->isTouching && !isPressed)
        return;
    is_iPhoneComic = this->is_iPhoneComic;
    this->touchStartY = touchY;
    this->isTouching = isPressed;
    if (!is_iPhoneComic)
        touchY = x;
    this->touchStartX = x;
    this->begPoint = touchY;
    if (isPressed)
    {
        this->isSnappingBack = isTransitioning;
        this->midPoint = isTransitioning;
        return;
    }
    endPoint = this->endPoint;
    if (is_iPhoneComic)
        halfScreen = 160;
    else
        halfScreen = 240;
    if (halfScreen < endPoint || (midPoint = this->midPoint, midPoint > 2))
    {
        isPhoneComic = !is_iPhoneComic;
        this->isSnappingBack = isTransitioning;
        if (is_iPhoneComic)
            page = this->page;
        else
            page = this->iPhonePage;
        if (!isPhoneComic)
        {
            nextPageIdx = page + 1;
        set_cur_page:
            this->curPage = nextPageIdx;
            goto clamp_cur_page;
        }
        prevPageIdx = page - 1;
    set_cur_iphone_page:
        this->cur_iPhonePage = prevPageIdx;
        goto clamp_cur_page;
    }
    if (endPoint < -halfScreen || midPoint < -2)
    {
        this->isSnappingBack = isTransitioning;
        if (is_iPhoneComic)
        {
            nextPageIdx = this->page - 1;
            goto set_cur_page;
        }
        prevPageIdx = this->iPhonePage + 1;
        goto set_cur_iphone_page;
    }
    this->curPage = this->page;
    if (endPoint)
        this->isSnappingBack = 1;
    this->cur_iPhonePage = this->iPhonePage;
    if (endPoint)
        this->isSnappingBack = 1;
clamp_cur_page:
    curPage = this->curPage;
    if (curPage < 0)
    {
        wrappedPage = 16;
    write_clamped_page:
        this->curPage = wrappedPage;
        goto clamp_iphone_page;
    }
    if (curPage > 16)
    {
        wrappedPage = 0;
        goto write_clamped_page;
    }
clamp_iphone_page:
    cur_iPhonePage = this->cur_iPhonePage;
    if (cur_iPhonePage >= 0)
    {
        if (cur_iPhonePage <= 38)
            return;
        wrappedIPhonePage = 0;
    }
    else
    {
        wrappedIPhonePage = 38;
    }
    this->cur_iPhonePage = wrappedIPhonePage;
}

bool ComicBook::ButtonTouch(int x, int y)
{
    int btnX;
    int btnY;
    int btnW;
    int btnH;
    int rightEdge;
    bool inXRange;

    this->exitBtnHighlighted = false;
    if (!this->drawExitButton)
        return false;
    btnX = this->exitBtnRect[0];
    btnY = this->exitBtnRect[1];
    btnW = this->exitBtnRect[2];
    btnH = this->exitBtnRect[3];
    if (this->is_iPhoneComic)
    {
        rightEdge = btnX + btnW;
        btnX = this->exitBtnRect[1];
        btnY = Applet::IOS_HEIGHT - rightEdge;
        btnH = this->exitBtnRect[2];
        btnW = this->exitBtnRect[3];
    }
    inXRange = btnX <= x;
    if (btnX <= x)
        inXRange = x <= btnX + btnW;
    if (!inXRange || btnY > y || y > btnY + btnH)
        return false;
    this->exitBtnHighlighted = true;
    return true;
}

void ComicBook::TouchMove(int x, int y)
{
    bool is_iPhoneComic;
    int prevBegPoint;
    int moveDelta;
    int endPoint;
    int begPoint;

    if (this->isLoaded && !this->exitCancelled)
    {
        this->ButtonTouch(x, y);
        if (!this->isTransitioning && this->isTouching)
        {
            is_iPhoneComic = this->is_iPhoneComic;
            this->curX = x;
            this->curY = y;
            if (is_iPhoneComic)
            {
                begPoint = this->begPoint;
                this->begPoint = y;
                moveDelta = y - begPoint;
            }
            else
            {
                prevBegPoint = this->begPoint;
                this->begPoint = x;
                moveDelta = x - prevBegPoint;
            }
            endPoint = this->endPoint;
            this->midPoint = moveDelta;
            this->endPoint = moveDelta + endPoint;
        }
    }
}

void ComicBook::DeleteImages() {
    this->isLoaded = false;
    for (int i = 0; i < 17; i++) {
        this->imgComicBook[i]->~Image();
        this->imgComicBook[i] = nullptr;
    }
    for (int i = 0; i < 39; i++) {
        this->imgiPhoneComicBook[i]->~Image();
        this->imgiPhoneComicBook[i] = nullptr;
    }
    this->comicBookIndex = 0;
    this->iPhoneComicIndex = 0;
    this->loadingCounter = 0;
}

void ComicBook::DrawExitButton(Graphics* graphics) {
    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;
    int btnX;
    int btnY;
    int btnW;
    int btnH;
    int rightEdge;
    int brightness;
    Text* SmallBuffer;
    int unused_flags;
    btnX = this->exitBtnRect[0];
    btnY = this->exitBtnRect[1];
    btnW = this->exitBtnRect[2];
    btnH = this->exitBtnRect[3];
    if (this->is_iPhoneComic)
    {
        rightEdge = btnX + btnW;
        btnX = this->exitBtnRect[1];
        btnY = Applet::IOS_HEIGHT - rightEdge;
        btnH = this->exitBtnRect[2];
        btnW = this->exitBtnRect[3];
    }
    if (this->exitBtnHighlighted) {
        brightness = 150;
    }
    else {
        brightness = 50;
    }
    graphics->fillRect(btnX - 1, btnY - 1, btnW + 2, btnH + 2, 0);
    graphics->fillRect(btnX, btnY, btnW, btnH, (brightness << 8) | (brightness << 16) | 0xFF);
    SmallBuffer = app->localization->getSmallBuffer();
    SmallBuffer->setLength(0);
    SmallBuffer->append("Done");
    SmallBuffer->dehyphenate();
    graphics->drawString(SmallBuffer, btnX + btnW / 2, btnY + btnH / 2, (this->is_iPhoneComic) ? 67 : 3);
    SmallBuffer->dispose();
}


void ComicBook::handleComicBookEvents(int key, int keyAction) {
    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;
    int cX = app->canvas->SCR_CX;
    int cY = app->canvas->SCR_CY;
    int i;

    LOG_INFO("[comicbook] handleComicBookEvents {}, {}\n", key, keyAction);

    CAppContainer::getInstance()->userPressed(cX, cY);
    if (keyAction == Enums::ACTION_LEFT) {
        for (i = 0; i < 64; i++) {
            CAppContainer::getInstance()->userMoved(cX++, cY);
        }
    }
    else if (keyAction == Enums::ACTION_RIGHT) {

        for (i = 0; i < 64; i++) {
            CAppContainer::getInstance()->userMoved(cX--, cY);
        }
    }
    CAppContainer::getInstance()->userReleased(cX, cY);
    this->drawExitButton = false;
}