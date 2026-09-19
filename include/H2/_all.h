#ifndef HOMM2__ALL_H
#define HOMM2__ALL_H
// HoMM2 locator declarations. Inheritance is omitted and classes use novtable
// so empty HoMM1 bodies emit their own symbols without inventing unlabeled
// base constructors, vtables, or deleting destructors.

#include <BASE/baseManager.h>
#include <BASE/resource.h>
#include <BASE/resourceManager.h>
#include <H2/_carcass_types.h>
#include <H2/_types.h>
#include <H2/BASE/bitmap.h>
#include <H2/BASE/border.h>
#include <H2/BASE/button.h>
#include <H2/BASE/dimmerWidget.h>
#include <H2/BASE/dropListWidget.h>
#include <H2/BASE/executive.h>
#include <H2/BASE/font.h>
#include <H2/BASE/heroWindow.h>
#include <H2/BASE/heroWindowManager.h>
#include <H2/BASE/icon.h>
#include <H2/BASE/iconWidget.h>
#include <H2/BASE/inputManager.h>
#include <H2/BASE/listBoxWidget.h>
#include <H2/BASE/MIDIWrap.h>
#include <H2/BASE/mouseManager.h>
#include <H2/BASE/palette.h>
#include <H2/BASE/sample.h>
#include <H2/BASE/soundManager.h>
#include <H2/BASE/textEntryWidget.h>
#include <H2/BASE/textWidget.h>
#include <H2/BASE/tileset.h>
#include <H2/BASE/widget.h>
#include <H2/EDITOR/fullMap.h>
#include <H2/SOURCE/advManager.h>
#include <H2/SOURCE/army.h>
#include <H2/SOURCE/armyGroup.h>
#include <H2/SOURCE/bankBox.h>
#include <H2/SOURCE/combatManager.h>
#include <H2/SOURCE/ExpCampaign.h>
#include <H2/SOURCE/fileRequester.h>
#include <H2/SOURCE/game.h>
#include <H2/SOURCE/hero.h>
#include <H2/SOURCE/hexcell.h>
#include <H2/SOURCE/highScoreManager.h>
#include <H2/SOURCE/philAI.h>
#include <H2/SOURCE/playerData.h>
#include <H2/SOURCE/recruitUnit.h>
#include <H2/SOURCE/searchArray.h>
#include <H2/SOURCE/strip.h>
#include <H2/SOURCE/swapManager.h>
#include <H2/SOURCE/town.h>
#include <H2/SOURCE/townManager.h>
#include <H2/SOURCE/townObject.h>

#endif // HOMM2__ALL_H
