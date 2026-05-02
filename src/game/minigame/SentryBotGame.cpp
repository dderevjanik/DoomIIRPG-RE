#include <stdexcept>

#include "CAppContainer.h"
#include "App.h"
#include "Combat.h"
#include "Text.h"
#include "Game.h"
#include "Button.h"
#include "Canvas.h"
#include "MinigameUI.h"
#include "Player.h"
#include "SentryBotGame.h"
#include "CombatEntity.h"
#include "MenuStrings.h"
#include "MenuSystem.h"
#include "Image.h"
#include "Sound.h"
#include "Enums.h"
#include "Utils.h"
#include "Menus.h"
#include "SoundNames.h"
#include "Sounds.h"

bool SentryBotGame::wasTouched = false;

SentryBotGame::SentryBotGame() {
	// Zero data members without clobbering the vtable pointer
	char* start = reinterpret_cast<char*>(&this->app);
	std::memset(start, 0, sizeof(SentryBotGame) - (start - reinterpret_cast<char*>(this)));
}

SentryBotGame::~SentryBotGame() {
}

void SentryBotGame::playFromMainMenu(){
	if (!this->app) this->app = CAppContainer::getInstance()->app;
	this->initGame(nullptr, 0);
}

void SentryBotGame::setupGlobalData() {
	if (!this->app) this->app = CAppContainer::getInstance()->app;
	this->stateVars = app->canvas->stateVars;
	app->canvas->setState(Canvas::ST_MINI_GAME);
	this->stateVars[0] = 0;
	this->stateVars[1] = 0;
	this->stateVars[2] = 0;
	app->minigameUI->initHelpScreen();
	app->canvas->clearSoftKeys();
}

void SentryBotGame::initGame(ScriptThread* scriptThread, short botType) {

    if (!this->app) this->app = CAppContainer::getInstance()->app;
    Applet* app = this->app;
    this->callingThread = scriptThread;
    this->gamePlayedFromMainMenu = (this->callingThread == nullptr) ? true : false;
    this->setupGlobalData();
    this->botType = botType;
    if (this->botType != 0 && this->botType != 1) {
        this->botType = 0;
    }
    int minSkipCode = 2;
    int maxSkipCode = 6;
    int iqPercent = this->gamePlayedFromMainMenu ? 0 : app->player->ce->getIQPercent();
    int skipCodeRange = (10000 * minSkipCode + (maxSkipCode - minSkipCode) * (100 - iqPercent) * (100 - iqPercent)) / 10000;
    do {
        this->skipCode = app->nextInt() % skipCodeRange + 1;
    } while (this->skipCode == app->player->lastSkipCode);
    app->player->lastSkipCode = (uint8_t)this->skipCode;
    bool allowDuplicates = this->gamePlayedFromMainMenu || app->player->ce->getIQPercent() < 50;
    for (int i = 0; i < 16; ++i) {
        this->gameBoard[i] = app->nextInt() % 6 + 1;
        if (this->gameBoard[i] == this->skipCode && !allowDuplicates) {
            --i;
        }
    }
    this->gameBoard[0] = this->skipCode % 6 + 1;
    this->player_cursor = -1;
    int player_cursor = this->player_cursor;
    for (int j = 0; j < 4; ++j) {
        for (player_cursor = (player_cursor + this->skipCode) % 16; this->gameBoard[player_cursor] == this->skipCode; player_cursor = (player_cursor + this->skipCode) % 16) {}
        this->solution[j] = this->gameBoard[player_cursor];
    }
    for (int k = 0; k < 4; ++k) {
        this->usersGuess[k] = 0;
    }
    this->answer_cursor = 0;
    this->bot_selection_cursor = 0;
    this->timeSinceLastCursorMove = app->time;
    this->failedEarly = false;
    
    if (this->imgSubmit) {
        this->imgSubmit->~Image();
        this->imgSubmit = nullptr;
    }
    if (this->imgUnk1) {
        this->imgUnk1->~Image();
        this->imgUnk1 = nullptr;
    }
    if (this->imgDelete) {
        this->imgDelete->~Image();
        this->imgDelete = nullptr;
    }

    this->imgSubmit = app->loadImage("matrixSkip_button.bmp", true);
    this->imgUnk1 = app->loadImage("matrixSkip_button.bmp", true);
    this->imgDelete = app->loadImage("matrixSkip_button.bmp", true);

    if (this->m_sentryBotButtons) {
        delete this->m_sentryBotButtons;
    }
    this->m_sentryBotButtons = nullptr;

    this->m_sentryBotButtons = new fmButtonContainer();

    fmButton* btnExit = new fmButton(0, 428, 290, 54, 30, 1027); // Old -> (0, 420, 260, 60, 60, 1027);
    this->m_sentryBotButtons->AddButton(btnExit);

    fmButton* btnBack = new fmButton(1, 0, 290, 54, 30, 1027); // Old -> (1, 0, 260, 60, 60, 1027);
    this->m_sentryBotButtons->AddButton(btnBack);

    fmButton* button03 = new fmButton(2, 368, 142 - this->imgSubmit->height / 2, this->imgSubmit->width, this->imgSubmit->height, 1027);
    button03->SetImage(this->imgSubmit, 0);
    button03->SetHighlightImage(this->imgSubmit, 0);
    button03->normalRenderMode = 12;
    button03->highlightRenderMode = 0;
    this->m_sentryBotButtons->AddButton(button03);

    fmButton* button04 = new fmButton(3, 8, 142 - this->imgDelete->height / 2, this->imgDelete->width, this->imgDelete->height, 1027);
    button04->SetImage(this->imgDelete, 0);
    button04->SetHighlightImage(this->imgDelete, 0);
    button04->normalRenderMode = 12;
    button04->highlightRenderMode = 0;
    this->m_sentryBotButtons->AddButton(button04);

    this->touched = false;
    this->currentPress_x = 0;
    this->currentPress_y = 0;
    SentryBotGame::wasTouched = false;
}

void SentryBotGame::handleInput(int action) {

    Applet* app = this->app;

    if ((this->stateVars[2] == 2 || (this->stateVars[2] == 1 && this->gamePlayedFromMainMenu)) && (action == 7 || action == 15 || action == 6)) {
        this->endGame(0);
        return;
    }

    if (this->stateVars[1] == 0) {
        if (action == Enums::ACTION_MENU/*Enums::ACTION_AUTOMAP */|| action == Enums::ACTION_BACK)
        {
            switch (this->stateVars[2]) {
                case 0: {
                    this->endGame(2);
                    break;
                }
                case 1: {
                    if (this->bot_selection_cursor == 0) {
                        this->awardSentryBot((this->botType == 0) ? 3 : 5);
                        this->endGame(1);
                        break;
                    }
                    this->awardSentryBot((this->botType == 0) ? 4 : 6);
                    this->endGame(1);
                    break;
                }
                case 2: {
                    this->endGame(0);
                    break;
                }
            }
            return;
        }
        else if (action == Enums::ACTION_FIRE)
        {
            this->stateVars[1] = 1;
            app->canvas->clearSoftKeys();
        }
        else if (action == Enums::ACTION_UP) {
            app->minigameUI->handleHelpScreenScroll(-1);
        }
        else if (action == Enums::ACTION_DOWN) {
            app->minigameUI->handleHelpScreenScroll(1);
        }
        else if (action == Enums::ACTION_RIGHT) {
            app->minigameUI->handleHelpScreenScroll(16);
        }
        else if (action == Enums::ACTION_LEFT) {
            app->minigameUI->handleHelpScreenScroll(-16);
        }
    }
    else if (this->stateVars[1] == 1) {
        if (this->stateVars[2] == 0) {
            switch (action)
            {
                case Enums::ACTION_AUTOMAP: // Old ACTION_MENU
                case Enums::ACTION_BACK: {
                    stateVars[1] = 0;
                    app->minigameUI->initHelpScreen();
                    app->canvas->clearSoftKeys();
                    break;
                }
                case Enums::ACTION_MENU: { // Old ACTION_AUTOMAP
                    this->endGame(2);
                    break;
                }
                case Enums::ACTION_FIRE: {
                    if (this->answer_cursor < 4) {
#if 0 // J2ME
                        ++this->answer_cursor;
                        if (!this->playerCouldStillWin()) {
                            app->canvas->clearSoftKeys();
                            this->stateVars[2] = 2;
                            this->stateVars[3] = app->upTimeMs;
                            this->failedEarly = true;
                            break;
                        }
#else
                        if (this->player_cursor > -1) {
                            this->usersGuess[this->answer_cursor] = this->gameBoard[this->player_cursor];
                            if (this->answer_cursor <= 3) {
                                this->answer_cursor++;
                            }
                            //this->player_cursor = -1; // [GEC] Block this line
                        }
#endif
                        break;
                    }
                    else {
                        if (this->playerHasWon()) {
                            app->canvas->clearSoftKeys();
                            this->stateVars[2] = 1;
                            this->stateVars[3] = app->upTimeMs;
                            break;
                        }
                        app->canvas->clearSoftKeys();
                        this->stateVars[2] = 2;
                        this->stateVars[3] = app->upTimeMs;
                        break;
                    }
                    break;
                }

                case Enums::ACTION_LEFT:
                    if (this->player_cursor > -1) {
                        this->player_cursor = (this->player_cursor + 15) % 16;
                        this->timeSinceLastCursorMove = app->time;
                        app->menuSystem->soundClick(); // [GEC]
                        break;
                    }
                    break;

                case Enums::ACTION_RIGHT:
                    this->player_cursor = (this->player_cursor + 1) % 16;
                    this->timeSinceLastCursorMove = app->time;
                    app->menuSystem->soundClick(); // [GEC]
                    break;

                case (Enums::ACTION_PASSTURN):
                case (Enums::ACTION_MATRIX_LEFT): {
                    if (this->answer_cursor > 0) {
                        if (this->answer_cursor <= 3) {
                            this->usersGuess[this->answer_cursor] = 0;
                        }
                        this->answer_cursor--;
                        if (this->answer_cursor == 0) {
                            //this->player_cursor = -1; // [GEC] Block this line
                        }
                        this->usersGuess[this->answer_cursor] = 0;
                    }
                    break;
                }
                case (Enums::ACTION_MATRIX_RIGHT): {
                    if (this->player_cursor > -1) {
                        if (this->answer_cursor <= 3) {
                            this->usersGuess[this->answer_cursor] = this->gameBoard[this->player_cursor];
                            this->answer_cursor++;
                        }
                        //this->player_cursor = -1; // [GEC] Block this line
                    }
                    break;
                }
            }
        }
        else if (this->stateVars[2] == 1) {
            if (!this->gamePlayedFromMainMenu) {
                switch (action) {
                    case Enums::ACTION_FIRE:
                    /*case Enums::ACTION_AUTOMAP:*/ {
                        if (this->bot_selection_cursor == 0) {
                            this->awardSentryBot((this->botType == 0) ? 3 : 5);
                            this->endGame(1);
                            break;
                        }
                        this->awardSentryBot((this->botType == 0) ? 4 : 6);
                        this->endGame(1);
                        break;
                    }
                    case Enums::ACTION_LEFT:
                    case Enums::ACTION_RIGHT: {
                        this->bot_selection_cursor = (this->bot_selection_cursor + 1) % 2;
                        break;
                    }
                }
            }
        }
    }
}

void SentryBotGame::updateGame(Graphics* graphics) {

    Applet* app = this->app;
    graphics->clipRect(0, 0, app->canvas->menuRect[2], app->canvas->menuRect[3]);
    graphics->setClipRect(0, 0, Applet::IOS_WIDTH, Applet::IOS_HEIGHT);
    if (this->stateVars[1])
    {
        switch (this->stateVars[2])
        {
        case 1:
            this->drawSuccessScreen(graphics);
            break;
        case 2:
            this->drawFailureScreen(graphics);
            break;
        case 0:
            this->drawGameScreen(graphics);
            break;
        }
    }
    else {
        this->drawHelpScreen(graphics);
    }
    app->canvas->staleView = 1;
    graphics->resetScreenSpace();
    
}

void SentryBotGame::drawFailureScreen(Graphics* graphics) {

    Applet* app = this->app;
    graphics->drawImage(this->imgMatrixSkip_BG, 0, 0, 0, 0, 0);
    app->canvas->setLeftSoftKey((short)0, (short)30);
    Text* smallBuffer = app->localization->getSmallBuffer();
    int centerX = (app->canvas->screenRect[2] - app->canvas->screenRect[0]) / 2;
    int posY = 0;
    this->drawPlayersGuess(centerX, posY + 26, false, smallBuffer, graphics);
    smallBuffer->setLength(0);
    app->localization->composeText((short)0, (short)(this->failedEarly ? 179 : 178), smallBuffer);
    smallBuffer->dehyphenate();
    graphics->drawString(smallBuffer, centerX, posY + 15, 3);
    posY += 59;
    smallBuffer->setLength(0);
    app->localization->composeText((short)0, (short)240, smallBuffer);
    smallBuffer->dehyphenate();
    graphics->drawString(smallBuffer, centerX, posY + 12, 3);
    short stringId;
    short labelY;
    if (this->botType == 1) {
        graphics->fillRect(centerX - 53, posY + 80 - 26, 107, 52, -10747893);
        graphics->fillRect(centerX - 50, posY + 80 - 23, 101, 46, -13354445);
        graphics->fillRect(centerX - 40, posY + 80 - 13, 80, 26, -10747893);
        graphics->fillRect(centerX - 33, posY + 80 - 6, 66, 12, -13354445);
        smallBuffer->setLength(0);
        app->localization->composeText((short)0, (short)186, smallBuffer);
        smallBuffer->dehyphenate();
        graphics->drawString(smallBuffer, centerX, posY + 50 + 2, 3);
        if (this->botType == 1) {
            labelY = 129;
            stringId = 187;
        }
        else {
            labelY = 109;
            stringId = 188;
        }
    }
    else {
        labelY = 59;
        stringId = 188;
    }

    smallBuffer->setLength(0);
    app->localization->composeText((short)0, stringId, smallBuffer);
    smallBuffer->wrapText(22, '\n');
    graphics->drawString(smallBuffer, centerX, labelY + 80, 3);
    smallBuffer->dispose();
}


void SentryBotGame::drawSuccessScreen(Graphics* graphics) {

    Applet* app = this->app;

    graphics->drawImage(this->imgMatrixSkip_BG, 0, 0, 0, 0, 0);
    app->canvas->setLeftSoftKey((short)0, (short)30);
    Text* smallBuffer = app->localization->getSmallBuffer();
    int centerX = (app->canvas->screenRect[2] - app->canvas->screenRect[0]) / 2;
    int posY = 0;
    int w = 0;
    this->drawPlayersGuess(centerX, posY + 26, false, smallBuffer, graphics);
    smallBuffer->setLength(0);
    app->localization->composeText((short)0, (short)177, smallBuffer);
    smallBuffer->dehyphenate();
    graphics->drawString(smallBuffer, centerX, posY + 15, 3);
    if (this->gamePlayedFromMainMenu) {
        posY += 59;
        smallBuffer->setLength(0);
        app->localization->composeText((short)0, (short)239, smallBuffer);
        smallBuffer->dehyphenate();
        graphics->drawString(smallBuffer, centerX, posY + 12, 3);
        smallBuffer->setLength(0);
        app->localization->composeText((short)0, (short)188, smallBuffer);
        smallBuffer->wrapText(22, '\n');
        graphics->drawString(smallBuffer, centerX, posY + 80, 3);

        if (SentryBotGame::wasTouched) {
            if (this->touched) {
                SentryBotGame::wasTouched = this->touched;
            }
            else {
                this->handleInput(Enums::ACTION_FIRE);
            }
        }
        else {
            SentryBotGame::wasTouched = this->touched;
        }
    }
    else {
        posY += 95;
        smallBuffer->setLength(0);
        app->localization->composeText((short)0, (short)180, smallBuffer);
        smallBuffer->dehyphenate();
        graphics->drawString(smallBuffer, centerX, posY, 3);
        this->m_sentryBotButtons->GetButton(2)->Render(graphics);
        this->m_sentryBotButtons->GetButton(3)->Render(graphics);
        //this->bot_selection_cursor = -1; // [GEC] Block this line

        smallBuffer->setLength(0);
        app->localization->composeText((short)0, (short)181, smallBuffer);
        smallBuffer->dehyphenate();
        w = smallBuffer->getStringWidth();
        if (this->m_sentryBotButtons->GetButton(3)->highlighted) {
            this->bot_selection_cursor = 0;
        }
        graphics->drawString(smallBuffer, centerX - 180, posY + 45, 3);

        int xPos = 0;
        if (this->bot_selection_cursor == 0) {
            xPos = ((centerX - 180) - (w /2)) - 5;
        }

        smallBuffer->setLength(0);
        app->localization->composeText( (short)0, (short)182, smallBuffer);
        smallBuffer->dehyphenate();
        w = smallBuffer->getStringWidth();
        if (this->m_sentryBotButtons->GetButton(2)->highlighted) {
            this->bot_selection_cursor = 1;
        }
        graphics->drawString(smallBuffer, centerX + 180, posY + 45, 3);

        if (this->bot_selection_cursor == 1) {
            xPos = ((centerX + 180) - (w / 2)) - 5;
        }

        graphics->drawCursor(xPos + app->canvas->OSC_CYCLE[(app->time / 100) % 4], posY + 45, 10); // [GEC] Restored from J2ME/BREW

        int panelTop = app->canvas->screenRect[3] - 10 - 140;
        int panelLeft = app->canvas->screenRect[0] + 12;
        graphics->fillRect(panelLeft, panelTop, app->canvas->screenRect[2] - app->canvas->screenRect[0] - 24, 140, 0xFF464C43);
        smallBuffer->setLength(0);
        smallBuffer->append("Shooting Bot:");
        smallBuffer->dehyphenate();
        graphics->drawString(smallBuffer, panelLeft + 10, panelTop + 8, 20);
        smallBuffer->setLength(0);
        app->localization->composeText((short)0, (short)183, smallBuffer);
        smallBuffer->dehyphenate();
        smallBuffer->wrapText(50, '\n');
        graphics->drawString(smallBuffer, panelLeft + 10, panelTop + 27, 20);

        smallBuffer->setLength(0);
        smallBuffer->append("Exploding Bot:");
        smallBuffer->dehyphenate();
        graphics->drawString(smallBuffer, panelLeft + 10, panelTop + 73, 20);
        smallBuffer->setLength(0);
        app->localization->composeText((short)0, (short)184, smallBuffer);
        smallBuffer->dehyphenate();
        smallBuffer->wrapText(50, '\n');
        graphics->drawString(smallBuffer, panelLeft + 10, panelTop + 92, 20);
    }
    smallBuffer->dispose();
}

void SentryBotGame::drawHelpScreen(Graphics* graphics) {

    Applet* app = this->app;
    app->minigameUI->drawHelpScreen(app->canvas.get(), graphics, Localization::STRINGID((short)0, (short)171), Localization::STRINGID((short)0, (short)172), this->imgHelpScreenAssets);
    Text* text = app->localization->getSmallBuffer();
    app->setFontRenderMode(0);;
    app->localization->composeText((short)0, (short)30, text); // Old -> text->append("Exit");
    if (!this->m_sentryBotButtons->GetButton(0)->highlighted) {
        app->setFontRenderMode(2);
    }
    graphics->drawString(text, 445, 302, 3); // Old -> 310, 3);
    app->setFontRenderMode(0);
    text->dispose();
}

void SentryBotGame::drawGameScreen(Graphics* graphics) {

    Applet* app = this->app;
    int centerX = (app->canvas->screenRect[2] - app->canvas->screenRect[0]) / 2;
    int gridY = app->canvas->screenRect[1];
    gridY += 50;
    Text* smallBuffer = app->localization->getSmallBuffer();
    graphics->drawImage(this->imgMatrixSkip_BG, 0, 0, 0, 0, 0);
    graphics->drawRegion(this->imgGameAssets, 53, 0, 91, 91, centerX - 91, gridY, 20, 0, 0);
    graphics->drawRegion(this->imgGameAssets, 53, 0, 91, 91, centerX, gridY, 20, 4, 0);
    graphics->drawRegion(this->imgGameAssets, 53, 0, 91, 91, centerX - 91, gridY + 91, 20, 6, 0);
    graphics->drawRegion(this->imgGameAssets, 53, 0, 91, 91, centerX, gridY + 91, 20, 2, 0);
    if (this->player_cursor == -1) {
        graphics->drawRegion(this->imgGameAssets, 144, 0, 41, 37, centerX - 120, gridY + 8, 20, 0, 0);
    }
    for (int i = 0; i < 16; ++i) {
        int tileX = centerX  + i % 4 * 42;
        int tileY = gridY + i / 4 * 44;
        smallBuffer->setLength(0);
        smallBuffer->append(this->gameBoard[i]);

        if (this->touched && pointInRectangle(this->currentPress_x, this->currentPress_y, tileX - 84, tileY + 6, 43-1, 43-1)) {
            int v9, rotateMode, posX, posY;
            v9 = tileX - 84;
            if (i != this->player_cursor) {
                app->menuSystem->soundClick();
            }
            this->player_cursor = i;

            Image* imgButton;
            switch (i)
            {
                case 0: {
                    imgButton = this->imgButton2;
                    rotateMode = 4;
                    posX = tileX - 84;
                    posY = tileY + 6;
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
                case 1: {
                    imgButton = this->imgButton;
                    rotateMode = 0;
                    posX = tileX - 85;
                    posY = tileY + 6;
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
                case 2: {
                    imgButton = this->imgButton;
                    rotateMode = 0;
                    posX = tileX - 84;
                    posY = tileY + 6;
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
                case 3: {
                    imgButton = this->imgButton2;
                    rotateMode = 0;
                    posX = tileX - 84;
                    posY = tileY + 6;
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
                case 4: {
                    imgButton = this->imgButton3;
                    rotateMode = 6;
                    posX = tileX - 84;
                    posY = tileY + 4;
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
                case 5:
                case 9:
                case 13: {
                    imgButton = this->imgButton;
                    rotateMode = 0;
                    posX = tileX - 86;
                    posY = tileY + 3;
                    if (i == 5) { // [GEC] new
                        posY = tileY + 4;
                    }
                    if (i == 13) { // [GEC] new
                        posY = tileY + 2;
                    }
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }

                case 6:
                case 10:
                case 14: {
                    imgButton = this->imgButton;
                    rotateMode = 0;
                    posX = tileX - 84;
                    posY = tileY + 3;
                    if (i == 6) { // [GEC] new
                        posY = tileY + 4;
                    }
                    if (i == 14) { // [GEC] new
                        posY = tileY + 2;
                    }
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
                case 7: {
                    imgButton = this->imgButton3;
                    rotateMode = 2;
                    posX = tileX - 84 + 1;
                    posY = tileY + 4; // Old 3
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
                case 8: {
                    imgButton = this->imgButton3;
                    rotateMode = 0;
                    posX = tileX - 84;
                    posY = tileY + 3;
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
                case 11: {
                    imgButton = this->imgButton3;
                    rotateMode = 4;
                    posX = tileX - 84 + 1;
                    posY = tileY + 3;
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
                case 12: {
                    imgButton = this->imgButton2;
                    rotateMode = 2;
                    posX = tileX - 84;
                    posY = tileY + 2; // Old 3
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
                case 15: {
                    imgButton = this->imgButton2;
                    rotateMode = 8; // Old 1
                    posX = tileX - 83;
                    posY = tileY + 2; // Old 3
                    graphics->drawImage(imgButton, posX, posY, 0, rotateMode, 0);
                    break;
                }
            }
        }

        graphics->drawString(smallBuffer, tileX - 63, tileY + 6 + 21, 3);
        if (this->player_cursor != -1 && this->player_cursor == i) {
           this->drawCursor(tileX - 63, tileY + 5 + 21, true, graphics);
        }
    }

    if (this->player_cursor == -1) {
        this->drawCursor(((centerX - 63) - 21) - 18, gridY + 5 + 21, true, graphics);
    }


    gridY += 199;
    this->drawPlayersGuess(centerX, gridY, true, smallBuffer, graphics);

    int topY = app->canvas->screenRect[1];
    smallBuffer->setLength(0);
    app->localization->composeText((short)0, (short)173, smallBuffer);
    app->localization->composeText((short)0, (short)174, smallBuffer);
    smallBuffer->append((int)this->skipCode);
    smallBuffer->dehyphenate();
    graphics->drawString(smallBuffer, centerX, topY + 30, 3);

    smallBuffer->setLength(0);
    app->localization->composeText((short)0, (short)175, smallBuffer);
    smallBuffer->dehyphenate();
    graphics->drawString(smallBuffer, centerX, topY + 241, 3);

    smallBuffer->setLength(0);
    app->localization->composeText((short)0, (short)176, smallBuffer);
    smallBuffer->dehyphenate();
    graphics->drawString(smallBuffer, centerX, topY + 294, 3);

    smallBuffer->setLength(0);
    smallBuffer->append("Back");
    if (!this->m_sentryBotButtons->GetButton(1)->highlighted) {
        app->setFontRenderMode( 2);
    }
    graphics->drawString(smallBuffer, 35, 310, 3);
    app->setFontRenderMode(0);

    smallBuffer->setLength(0);
    smallBuffer->append("Exit");
    if (!this->m_sentryBotButtons->GetButton(0)->highlighted) {
        app->setFontRenderMode(2);
    }
    graphics->drawString(smallBuffer, 445, 310, 3);
    app->setFontRenderMode(0);
    smallBuffer->dispose();

    if (this->answer_cursor > 3) {
        this->m_sentryBotButtons->GetButton(2)->normalRenderMode = 1;
        this->m_sentryBotButtons->GetButton(2)->highlightRenderMode = 0;
    }
    else {
        this->m_sentryBotButtons->GetButton(2)->normalRenderMode = 0;
        this->m_sentryBotButtons->GetButton(2)->highlightRenderMode = 0;
    }

    if (this->answer_cursor <= 0) {
        this->m_sentryBotButtons->GetButton(3)->normalRenderMode = 0;
        this->m_sentryBotButtons->GetButton(3)->highlightRenderMode = 0;
    }
    else {
        this->m_sentryBotButtons->GetButton(3)->normalRenderMode = 1;
        this->m_sentryBotButtons->GetButton(3)->highlightRenderMode = 0;
    }

    this->m_sentryBotButtons->GetButton(2)->Render(graphics);
    this->m_sentryBotButtons->GetButton(3)->Render(graphics);

    int x = this->m_sentryBotButtons->GetButton(3)->touchArea.x + (this->m_sentryBotButtons->GetButton(3)->touchArea.w / 2);
    int y = this->m_sentryBotButtons->GetButton(3)->touchArea.y + (this->m_sentryBotButtons->GetButton(3)->touchArea.h / 2);

    smallBuffer->setLength(0);
    smallBuffer->append("Delete");
    //if (!this->m_sentryBotButtons->GetButton(3)->highlighted) {
        //app->setFontRenderMode( 2);
    //}
    graphics->drawString(smallBuffer, x, y, 3);
    app->setFontRenderMode(0);

    x = this->m_sentryBotButtons->GetButton(2)->touchArea.x + (this->m_sentryBotButtons->GetButton(2)->touchArea.w / 2);
    y = this->m_sentryBotButtons->GetButton(2)->touchArea.y + (this->m_sentryBotButtons->GetButton(2)->touchArea.h / 2);
    
    smallBuffer->setLength(0);
    smallBuffer->append("Submit");
    //if (!this->m_sentryBotButtons->GetButton(2)->highlighted) {
        //app->setFontRenderMode(2);
    //}
    graphics->drawString(smallBuffer, x, y, 3);
    app->setFontRenderMode(0);
}

void SentryBotGame::drawPlayersGuess(int centerX, int topY, bool showCursor, Text* text, Graphics* graphics) {

    Applet* app = this->app;
    graphics->drawRegion(this->imgGameAssets, 0, 91, 95, 33, centerX - 95, topY, 20, 0, 0);
    graphics->drawRegion(this->imgGameAssets, 0, 91, 95, 33, centerX, topY, 20, 4, 0);
    int digitY = topY + 12 + 4;
    for (int i = 0; i < 4; ++i) {
        int digitX = centerX - 67 + i % 4 * 45;
        int digit = this->usersGuess[i];
        if (digit != 0) {
            text->setLength(0);
            text->append(digit);
            graphics->drawString(text, digitX, digitY, 3);
        }
        graphics->drawLine(digitX - 6, digitY + 8, digitX + 6, digitY + 8, 0xFF6C834A);
        if (showCursor && this->answer_cursor == i) {
            this->drawCursor(digitX, topY + 12 + 4, false, graphics);
        }
    }
}

void SentryBotGame::drawCursor(int x, int y, bool blinking, Graphics* graphics) {

    Applet* app = this->app;
    if (!blinking || (app->time - this->timeSinceLastCursorMove & 0x200) == 0x0) {
        graphics->drawRegion(this->imgGameAssets, 95, 91, 22, 22, x + 1, y, 3, 0, 0);
    }
}

bool SentryBotGame::playerHasWon() {
    for (int i = 0; i < 4; ++i) {
        if (this->usersGuess[i] != this->solution[i]) {
            return false;
        }
    }
    return true;
}

bool SentryBotGame::playerCouldStillWin() {
    for (int i = 0; i < 4; ++i) {
        if (this->usersGuess[i] != 0 && this->usersGuess[i] != this->solution[i]) {
            return false;
        }
    }
    return true;
}

void SentryBotGame::forceWin() {

    Applet* app = this->app;
    for (int i = 0; i < 4; ++i) {
        this->usersGuess[i] = this->solution[i];
    }
    this->stateVars[1] = 1;
    this->stateVars[2] = 1;
    app->canvas->clearSoftKeys();
}

void SentryBotGame::awardSentryBot(int weaponIdx) {

    Applet* app = this->app;

    for (int i = 0; i < app->combat->familiarDefCount; i++) {
        int wpIdx = app->combat->familiarDefs[i].weaponIndex;
        if ((app->player->weapons & (1 << wpIdx)) != 0) {
            app->player->attemptToDiscardFamiliar(wpIdx);
            app->player->showHelp((short)16, true);
            break;
        }
    }
    app->player->give(1, weaponIdx, 1);
}

void SentryBotGame::endGame(int result) {

    Applet* app = this->app;

    app->sound->playSound((result == 1) ? 1043 : 1040, 0, 3, 0);

    if (!this->gamePlayedFromMainMenu) {
        app->minigameUI->evaluateResults(result);
        app->game->scriptStateVars[7] = (short)result;
    }
    if (!this->gamePlayedFromMainMenu) {
        app->canvas->setState(Canvas::ST_PLAYING);
        this->callingThread->run();
        this->callingThread = nullptr;
    }
    else {
        app->sound->playSound(Sounds::getResIDByName(SoundName::MUSIC_TITLE), 1u, 3, 0);
        app->menuSystem->setMenu(Menus::MENU_MAIN_MINIGAME);
    }
}

void SentryBotGame::touchStart(int pressX, int pressY) {
    this->m_sentryBotButtons->HighlightButton(pressX, pressY, true);
    this->touched = true;
    this->currentPress_x = pressX;
    this->currentPress_y = pressY;
}

void SentryBotGame::touchMove(int pressX, int pressY) {
    this->m_sentryBotButtons->HighlightButton(pressX, pressY, true);
    this->currentPress_x = pressX;
    this->currentPress_y = pressY;
}

void SentryBotGame::touchEnd(int pressX, int pressY) {
    this->m_sentryBotButtons->HighlightButton(0, 0, false);
    this->touched = false;
    this->currentPress_x = pressX;
    this->currentPress_y = pressY;
    if (this->stateVars[1] == 0) {
        this->handleTouchForHelpScreen(pressX, pressY);
    }
    else {
        this->handleTouchForGame(pressX, pressY);
    }
}

void SentryBotGame::handleTouchForHelpScreen(int pressX, int pressY) {
    if (this->m_sentryBotButtons->GetTouchedButtonID(pressX, pressY) == 0) {
        this->handleInput(Enums::ACTION_MENU); // Old Enums::ACTION_AUTOMAP
    }
    else {
        this->handleInput(Enums::ACTION_FIRE);
    }
}

void SentryBotGame::handleTouchForGame(int pressX, int pressY) {
    int buttonID;

    if (this->stateVars[2] == 0) {
        buttonID = this->m_sentryBotButtons->GetTouchedButtonID(pressX, pressY);
        if (buttonID == 0) {
            this->handleInput(Enums::ACTION_MENU); // Old Enums::ACTION_AUTOMAP
        }
        else if (buttonID == 1) {
            this->handleInput(Enums::ACTION_BACK);
        }
        else if (buttonID == 2) {
            if (this->answer_cursor > 3) {
                this->handleInput(Enums::ACTION_FIRE);
            }
        }
        else if (buttonID == 3) {
            this->handleInput(Enums::ACTION_MATRIX_LEFT);
        }
        else {
            this->handleInput(Enums::ACTION_MATRIX_RIGHT);
        }
    }
    else if (this->stateVars[2] == 1) {
        if (this->bot_selection_cursor != -1) {
            this->handleInput(Enums::ACTION_FIRE);
        }
    }
    else if (this->stateVars[2] == 2) {
        this->handleInput(Enums::ACTION_MENU); // Old Enums::ACTION_AUTOMAP
    }
}
