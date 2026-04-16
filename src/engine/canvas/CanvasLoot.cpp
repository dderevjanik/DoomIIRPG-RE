#include <stdexcept>
#include <algorithm>
#include "Log.h"

#include "App.h"
#include "CAppContainer.h"
#include "Canvas.h"
#include "Game.h"
#include "Player.h"
#include "Combat.h"
#include "Entity.h"
#include "EntityDef.h"
#include "Text.h"
#include "Enums.h"
#include "LootComponent.h"

// poolLoot stays on Canvas — called from handlePlayingEvents in Canvas.cpp
void Canvas::poolLoot(int* array) {

	Entity* entity = app->game->findMapEntity(array[0], array[1], 512);
	this->lootText = app->localization->getLargeBuffer();
	this->lootText->setLength(0);
	this->numPoolItems = 0;
	this->numLootItems = 0;
	this->lootLineNum = 0;
	this->lootPoolCredits = 0;
	while (entity != nullptr) {
		if (entity->def->eType == Enums::ET_CORPSE) {
			if (!entity->isMonster()) {
				if (entity->param != 0) {
					entity = entity->nextOnTile;
					continue;
				}
				++entity->param;
			}
			else {
				if ((entity->monsterFlags & 0x800) != 0x0) {
					entity = entity->nextOnTile;
					continue;
				}
				entity->monsterFlags |= 0x800;
			}
			entity->info |= 0x400000;
			for (int i = 0; i < 3; ++i) {
				if (entity->loot->lootSet[i] == 0) {
					break;
				}
				bool b = true;
				int n = entity->loot->lootSet[i] >> 12 & 0xF;
				if (n == 6) {
					int n2 = entity->loot->lootSet[i] & 0xFFF;
					for (int j = 0; j < this->numPoolItems; ++j) {
						if ((entity->loot->lootSet[j] >> 12 & 0xF) == 0x6 && n2 == (this->lootPool[j] & 0xFFF)) {
							b = false;
							break;
						}
					}
				}
				else {
					int n3 = entity->loot->lootSet[i] & 0x3F;
					++this->numLootItems;
					int n4 = (entity->loot->lootSet[i] & 0xFC0) >> 6;
					if (n == 0) {
						if (n4 == 24) {
							this->lootPoolCredits += n3;
							continue;
						}
						if (n4 == 25) {
							this->lootPoolCredits += n3 * 100;
							continue;
						}
					}
					int n5 = entity->loot->lootSet[i] >> 6;
					for (int k = 0; k < this->numPoolItems; ++k) {
						if (n5 == this->lootPool[k] >> 6) {
							b = false;
							this->lootPool[k] = ((this->lootPool[k] & 0xFFFFFFC0) | (n3 + (this->lootPool[k] & 0x3F) & 0x3F));
							break;
						}
					}
				}
				if (b) {
					this->lootPool[this->numPoolItems++] = entity->loot->lootSet[i];
				}
			}
		}
		entity = entity->nextOnTile;
	}
	for (int l = 0; l < this->numPoolItems; ++l) {
		int n6 = this->lootPool[l];
		int n7 = n6 >> 12 & 0xF;
		if (n7 == 6) {
			short n8 = (short)(n6 & 0xFFF);
			const auto& trinketStrings = CAppContainer::getInstance()->gameConfig.trinketStrings;
			this->lootText->append('\x88');
			if (n8 >= 0 && n8 < (int)trinketStrings.size()) {
				this->lootText->append(trinketStrings[n8].c_str());
			}
			this->lootText->append("|");
		}
		else {
			int n9 = (n6 & 0xFC0) >> 6;
			int n10 = n6 & 0x3F;
			app->localization->resetTextArgs();
			app->localization->addTextArg('\x88');
			EntityDef* find = app->entityDefManager->find(6, n7, n9);
			if (n7 == 1) {
				app->localization->addTextArg((short)1, find->longName);
				app->localization->composeText((short)0, (short)91, this->lootText);
			}
			else {
				app->localization->addTextArg(n10);
				app->localization->addTextArg((short)1, find->longName);
				app->localization->composeText((short)0, (short)90, this->lootText);
			}
		}
	}
	if (this->lootPoolCredits != 0) {
		app->localization->resetTextArgs();
		app->localization->addTextArg('\x88');
		app->localization->addTextArg(this->lootPoolCredits);
		app->localization->addTextArg((short)1, (short)157);
		app->localization->composeText((short)0, (short)90, this->lootText);
	}
	if (this->numPoolItems == 0 && this->lootPoolCredits == 0) {
		app->localization->composeText((short)0, (short)228, this->lootText);
	}
	this->lootText->dehyphenate();
	for (int n13 = 0; n13 < 18; ++n13) {
		this->lootPoolIndices[n13] = (short)0;
	}
	int length = this->lootText->length();
	int n11 = 0;
	int n12 = 0;
	for (int n13 = 0; n13 < length; ++n13) {
		if (this->lootText->charAt(n13) == '|') {
			this->lootPoolIndices[n12 * 2] = (short)n11;
			this->lootPoolIndices[n12 * 2 + 1] = (short)(n13 - n11);
			++n12;
			n11 = n13 + 1;
		}
	}
	this->lootPoolIndices[n12 * 2] = (short)n11;
	this->lootPoolIndices[n12 * 2 + 1] = (short)(length - n11);
}
