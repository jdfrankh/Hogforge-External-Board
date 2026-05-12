#include "Menu.h"

static Menu* menuInstance = nullptr;

namespace {
String compactFileName(const String& fullPath, size_t maxLen = 18) {
    int lastSlash = fullPath.lastIndexOf('/');
    String fileName = (lastSlash >= 0) ? fullPath.substring(lastSlash + 1) : fullPath;

    if (fileName.length() <= static_cast<int>(maxLen)) {
        return fileName;
    }

    if (maxLen <= 3) {
        return fileName.substring(0, maxLen);
    }

    return fileName.substring(0, maxLen - 3) + "...";
}
}

Menu::Menu(RotaryEncoder& enc) 
    :  encoder(&enc), sdGcode(BUILTIN_SDCARD) {

        menuInstance = this;

        currentSelection = 0;
        visibleTop = 0;
        encoder->setPosition(0);
         
}

void Menu::begin() {
    Serial.println("Menu Init..");
    //display = new u8g2
    //(U8G2_R0, /* clock=*/ 24, /* data=*/ 25, /* reset=*/ U8X8_PIN_NONE);
    this->u8g2 = new U8G2_SSD1306_128X64_NONAME_F_SW_I2C(U8G2_R0, /* clock=*/ 0, /* data=*/ 1, /* reset=*/ U8X8_PIN_NONE); // -- works 
    //this->u8g2 = new U8G2_SSD1306_128X64_NONAME_1_HW_I2C (U8G2_R0, /* clock=*/ 24, /* data=*/ 25, /* reset=*/ U8X8_PIN_NONE);
    //this->u8g2 = new U8G2_SSD1306_128X64_NONAME_1_2ND_HW_I2C(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); // -- works
    this->u8g2->begin();
    this->u8g2->setFont(u8g2_font_t0_12b_mf);
    this->u8g2->setColorIndex(1);
    this->drawLogo();

    this->encoder->begin();
    this->encoder->setMinMax(0, menuItemCount - 1);
    sdReady = sdGcode.begin();


   Lnode* head = new Lnode("Main Menu", NULL,  MenuActions::MAIN_MENU); //0
    addNode(head, Lnode("Print", MenuActions::MAIN_MENU, MenuActions::PRINT_MENU)); //1
    addNode(head, Lnode("Settings", MenuActions::MAIN_MENU, MenuActions::SETTINGS_MENU)); //2
    addNode(head, Lnode("Tools", MenuActions::MAIN_MENU, MenuActions::TOOLS_MENU)); //3
    addNode(head, Lnode("About",MenuActions::MAIN_MENU, MenuActions::  ABOUT_MENU)); //4
    addNode(head, Lnode("Printer Info",MenuActions::ABOUT_MENU, MenuActions::PRINT_INFO)); //5
    addNode(head, Lnode("Print Cube",MenuActions::PRINT_MENU,MenuActions::PRINT_CUBE)); //6
    addNode(head, Lnode("Print From SD",MenuActions::PRINT_MENU,MenuActions::GET_GCODE_FILES)); //6
    addNode(head, Lnode("Set Vaccum Chamber",MenuActions::SETTINGS_MENU,MenuActions::PUMP_CHAMBER)); //6
    addNode(head, Lnode("Fiber Calibration",MenuActions::SETTINGS_MENU, MenuActions::CALIBRATE_FIBER)); //7
    addNode(head, Lnode("Vacuum Calibration",MenuActions::TOOLS_MENU, MenuActions::CALIBRATE_VACUUM)); //8
    addNode(head, Lnode("Toggle Laser Settings",MenuActions::TOOLS_MENU, MenuActions::PRINT_TOGGLE_LASER)); //10
    addNode(head, Lnode("Test Pushbar",MenuActions::TOOLS_MENU, MenuActions::MOVE_BAR)); 
    addNode(head, Lnode("Debug Controls",MenuActions::TOOLS_MENU, MenuActions::PRINT_DEBUG)); 
    addNode(head, Lnode("Run Through Step by Step",MenuActions::PRINT_DEBUG, MenuActions::SMALL_STEP)); 
    addNode(head, Lnode("Do One Cycle",MenuActions::PRINT_DEBUG, MenuActions::ONE_CYCLE)); 
    addNode(head, Lnode("Turn on Guide Laser",MenuActions::PRINT_TOGGLE_LASER, MenuActions::ACTIVATE_GUIDE_LASER)); //9
    addNode(head, Lnode("Turn off Guide Laser",MenuActions::PRINT_TOGGLE_LASER, MenuActions::DEACTIVATE_GUIDE_LASER)); //9
    addNode(head, Lnode("Turn on Fiber Laser",MenuActions::PRINT_TOGGLE_LASER, MenuActions::ACTIVATE_FIBER_LASER)); //10
    addNode(head, Lnode("Turn off Fiber Laser",MenuActions::PRINT_TOGGLE_LASER, MenuActions::DEACTIVATE_FIBER_LASER)); //10
    addNode(head, Lnode("Homing Settings",MenuActions::SETTINGS_MENU, MenuActions::PRINT_HOMING)); //11
    addNode(head, Lnode("Home All",MenuActions::PRINT_HOMING, MenuActions::HOME_ALL)); //8
    addNode(head, Lnode("Home Pushbar",MenuActions::PRINT_HOMING, MenuActions::HOME_PUSHBAR)); //8
    addNode(head, Lnode("Home Plates",MenuActions::PRINT_HOMING, MenuActions::HOME_PLATES)); //8
    addNode(head, Lnode("Prepare Print",MenuActions::PRINT_HOMING, MenuActions::PREP_PRINT)); 

    Tnode* root = convert(head);
    this->rootNode = root;
    this->currentNode = root;

    // store pointer to this instance so static callback functions can access members
    

    Serial.println("Menu end..");
}


void Menu::drawLogo(){
    this->u8g2->clearBuffer();
    this->u8g2->drawXBMP(0, 0, 128, 64, RIOTLogo);
    this->u8g2->sendBuffer();
    delay(2000);  // Show logo for 2 seconds
    

}

void::Menu::printFoundI2C(int connected){
    this->u8g2->clearBuffer();
    this->u8g2->drawStr(14, 10, connected % 10 == 1 ? "DB CONNECTED!" : "ER:DB NOT CONN");

    if((connected/100) % 10 ){
        this->u8g2->drawStr(34, 50,"Unkown Item Found!");

    }
    this->u8g2->sendBuffer();
    delay(2000);  // Show logo for 2 seconds
}

void Menu::U8G2EZ_init(){
    this->u8g2->begin();
    this->u8g2->setFont(u8g2_font_t0_12b_mf);
    this->u8g2->setColorIndex(1);
    this->drawLogo();
    Serial.println("Printing logo:");
}

void Menu::update(bool buttonState) {
    int newPosition = encoder->getPosition();

    if (gcodePrintActive) {
        this->encoder->setMinMax(0, 1);
        this->currentSelection = this->encoder->getPosition();
        if (buttonState) {
            if (this->currentSelection == 0) {
                requestCommand = MenuActions::TOGGLE_PAUSE_SD_GCODE_PRINT;
            } else if (this->currentSelection == 1) {
                requestCommand = MenuActions::CANCEL_SD_GCODE_PRINT;
            }
        }
        printGcodeStatus();
        return;
    }

    if(!alarm){
        if(confirmCompletion){
            confirmCompletion = 0;
            requestCommand = 0;
            back();
        }

        if (newPosition != this->currentSelection) {
            this->currentSelection = newPosition;
            Serial.printf("Encoder moved to %d\n", this->currentSelection);
        }

        if(buttonState){
            if (this->currentNode->act == MenuActions::GET_GCODE_FILES) {
                int selected = this->encoder->getPosition();
                if (selected == 0) {
                    back();
                    return;
                }

                int fileIndex = selected - 1;
                if (fileIndex >= 0 && fileIndex < static_cast<int>(gcodeFiles.size())) {
                    selectedGcodePath = gcodeFiles[fileIndex];
                    requestCommand = MenuActions::START_SD_GCODE_PRINT;
                    gcodePrintActive = true;
                    gcodePrintPaused = false;
                    gcodeCurrentLine = 0;
                    gcodeTotalLines = 0;
                    gcodeCurrentLineText = compactFileName(selectedGcodePath, 24);
                    goToMainMenu();
                    this->encoder->setPosition(0);
                    this->currentSelection = 0;
                }
                return;
            }

            if(this->currentSelection == 0 && this->currentNode->back != nullptr){
                back();
                return;
            }

            currentNode = currentNode->children[(this->currentNode->back != nullptr ? currentSelection-1 : currentSelection)];
            if (currentNode->act == MenuActions::GET_GCODE_FILES) {
                refreshGcodeFiles();
                visibleTop = 0;
                encoder->setPosition(0);
            }
        }
        else{
            actionSwitch(this->currentNode->act);
        }
    }
    else{
        this->u8g2->clearBuffer();
        this->u8g2->drawStr(34, 10, "An error has occured");
        this->u8g2->drawStr(34, 20, "Please restart");
        this->u8g2->drawStr(34, 30, "If issue persists");
        this->u8g2->drawStr(34, 40, "Contact an admin");
        this->u8g2->drawStr(14, 50, "Error Code:");
        char alarmStr[16];
        snprintf(alarmStr, sizeof(alarmStr), "%d", alarm);
        this->u8g2->drawStr(80, 50, alarmStr);
        this->u8g2->sendBuffer();
    }
}

// Converts a given linked list representing a complete 
// binary tree into the linked representation of a binary tree.
Tnode* Menu::convert(Lnode* head) {

    Tnode* prev = nullptr;

    if (head == nullptr) {
        return nullptr;
    }

    // Queue to store the parent nodes
    std::queue<Tnode*> q;

    // The first node is always the root node,
    // and add it to the queue
    Tnode* root = new Tnode(head->data , head->storeLevel ,  head->act);
    q.push(root);
    head = head->next;

    Serial.print("Root Node: "); Serial.println(root->data);
    prev = root;

    // Move the pointer to the next node
    while (head) {
        Serial.println("---------------------------");
        Tnode* parent = q.front();
        q.pop();
        Serial.print("Current Node: "); Serial.println(head->data);

        //currNode->back = prev;

        Tnode*temp = findNode(root, head->storeLevel);
        Tnode*child = new Tnode(head->data, head->storeLevel, head->act);

        if(temp){
            Serial.print("Found Parent: "); Serial.println(temp->data);
            
            child->back = temp;
            temp->children.push_back(child);

        } else {
            Serial.println("Parent not found");
            root->children.push_back(child);
        }
        //Serial.println("Looping through children: ");
        head = head->next;
        }

        return root; 
    }


void Menu::addNode(Lnode*& head, Lnode next) {
    if (head == nullptr) {
        head = new Lnode(next.data, next.storeLevel, next.act);
        return;
    }

    Lnode* curr = head;
    while (curr->next != nullptr) {
        curr = curr->next;
    }
    curr->next = new Lnode(next.data, next.storeLevel, next.act);
}

Tnode* Menu::findNode(Tnode* root, int targetAction) {
    if (!root) return nullptr;

    if (root->act == targetAction) {
        return root;
    }

    for (Tnode* child : root->children) {
        Tnode* result = findNode(child, targetAction);
        if (result) return result;
    }

    return nullptr;
}


// Level Order Traversal of the binary tree
void Menu::levelOrderTraversal(Tnode* root) {
    if (root == nullptr) {
        return;
    }

    // Queue to hold nodes at each level
    std::queue<Tnode*> q;
    q.push(root);

    while (!q.empty()) {
        Tnode* currNode = q.front();
        q.pop();

        // Print the current node's data
        Serial.print(currNode->data); Serial.print(" ");

    }
}

void Menu::actionSwitch(int actionNum){
    switch(actionNum){
        case MenuActions::MAIN_MENU:
            printMainMenu();
            break;
        case MenuActions::SETTINGS_MENU:
            printSettings();
            break;
        case MenuActions::TOOLS_MENU:
            printTools();
            break;
        case MenuActions::ABOUT_MENU:
            printAbout();
            break;
        case MenuActions::PRINT_MENU:
            printPrint();
            break;
        case MenuActions::PRINT_INFO:
            printInfo();
            break;
        case MenuActions::PRINT_HOMING:
            printHoming();
            break;
        case MenuActions::PUMP_CHAMBER:
            break;
        case MenuActions::CALIBRATE_FIBER:

            break;
        case MenuActions::CALIBRATE_VACUUM:
            break;
        case MenuActions::ACTIVATE_GUIDE_LASER:
            activateGuideLaser();
            break;
        case MenuActions::DEACTIVATE_GUIDE_LASER:
            deactivateGuideLaser();
        case MenuActions::ONE_CYCLE:
            oneCycle();
            break;
        case MenuActions::PRINT_CUBE:
            printCube();
            break;
        case MenuActions::ACTIVATE_FIBER_LASER:
            activateFiberLaser();
            break;
        case MenuActions::DEACTIVATE_FIBER_LASER:
            deactivateFiberLaser();
            break;
        case MenuActions::PRINT_TOGGLE_LASER:
            printToggle();
            break; 
        case MenuActions::HOME_PUSHBAR:
            homePush();
            break;
        case MenuActions::PRINT_DEBUG:
            printDebug();
            break;
        case MenuActions::HOME_PLATES:
            homePlates();
            break;
        case MenuActions::PREP_PRINT:
            prepPrint();
            break;
        case MenuActions::MOVE_BAR:
            movePushBar();
            break;
        case MenuActions::HOME_ALL:
            homeAll();
            break;
        case MenuActions::SMALL_STEP:
            smallStep();
            break;
        case MenuActions::GET_GCODE_FILES:
            printGcodeFiles();
            break;
        
        default:
            Serial.println("No action assigned");
            back();
            break;
    }
}

void Menu::printMainMenu( ){
    this->u8g2->clearBuffer();
    //Serial.println("Main Menu Selected");
    this->u8g2->setFont(u8g2_font_helvR08_tf); // Set any readable font
    this->u8g2->drawStr(0, 10, "Main Menu");
    this->u8g2->drawLine(0, 12, 75, 12);
    this->encoder->setMinMax(0, this->currentNode->children.size() - 1);

    this->encoder->setWrapAround(true);

    std::vector<String> children = this->currentNode->getChildrenNames();

    for(int i = 0; i < children.size(); i++){
         this->u8g2->drawButtonUTF8(128/2, 10 + (i+1)*12, (encoder->getPosition() == i ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV)  , 34 , 2 ,0, children[i].c_str());
    } 
    
    this->u8g2->sendBuffer();


  //  drawSimple("Main Menu", 1000, 0, 30, true);
}

void Menu::printSettings(){
    this->printChildren(this->currentNode, "Settings Menu");
   // drawSimple("Settings", 1000, 0, 30, true);
}

void Menu::printTools(){
    this->printChildren(this->currentNode, "Tools Menu");
   // drawSimple("Tools", 1000, 0, 30, true);
}
void Menu::printAbout(){
    this->printChildren(this->currentNode, "About Menu");
    
   // drawSimple("About", 1000, 0, 30, true);
}
void Menu::printDebug(){
    this->printChildren(this->currentNode, "Debug Menu");
    
   // drawSimple("About", 1000, 0, 30, true);
}
void Menu::printInfo(){
    this->printChildren(this->currentNode, "About Menu", false, false);
    this->u8g2->drawStr(34, 30, "Project Raycast");
    this->u8g2->drawStr(34, 40, "RIOT Robotics");
    this->u8g2->drawStr(34, 50, "Version 0.1");
    this->u8g2->sendBuffer();
    delay(20);
   // drawSimple("About", 1000, 0, 30, true);
}

void Menu::printPrint(){
    this->printChildren(this->currentNode, "Print Menu");
   // Serial.println("Print Selected");
   // drawSimple("Print", 1000, 0, 30, true);
}

void Menu::printToggle(){
    this->printChildren(this->currentNode, "Toggle Lasers");
}

void Menu::printHoming(){
    this->printChildren(this->currentNode, "Homing Calibration");
   // Serial.println("Print Selected");
   // drawSimple("Print", 1000, 0, 30, true);
}

void Menu::smallStep(){
    this->encoder->setPosition(1); // Always on back
    requestCommand = SMALL_STEP;
}

void Menu::oneCycle(){
    this->encoder->setPosition(2); 
    requestCommand = ONE_CYCLE;
    
}

void Menu::activateFiberLaser(){
   // Serial.println("Activating Guide Laser");
    this->encoder->setPosition(0); // Always on back

    requestCommand = ACTIVATE_FIBER_LASER;

    this->u8g2->clearBuffer();
    this->u8g2->drawButtonUTF8(105, 10, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");
    this->u8g2->drawButtonUTF8(128/2,20,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "The Fiber Laser");
    this->u8g2->drawButtonUTF8(128/2,30,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Will Turn on in 10s");
    this->u8g2->drawButtonUTF8(128/2,40,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "It will stay on for 3s");
    this->u8g2->drawButtonUTF8(128/2,50,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Please wait...");

    this->u8g2->sendBuffer();

}

void Menu::deactivateFiberLaser(){
    this->encoder->setPosition(0); // Always on back

    requestCommand = DEACTIVATE_FIBER_LASER;

    this->u8g2->clearBuffer();
    this->u8g2->drawButtonUTF8(105, 10, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");
    this->u8g2->drawButtonUTF8(128/2,20,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Turning Laser Off");


    this->u8g2->sendBuffer();

}


void Menu::activateGuideLaser(){
   // this->printChildren(this->currentNode, "Print Menu", false);
    //Serial.println("Activating Guide Laser");
    this->encoder->setPosition(0); // Always on back

    

    requestCommand = ACTIVATE_GUIDE_LASER; // Request the main to activate the guide laser function
    this->u8g2->clearBuffer();
    this->u8g2->drawButtonUTF8(105, 10, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");
    this->u8g2->drawButtonUTF8(128/2,20,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Toggling Guide Laser");
    this->u8g2->drawButtonUTF8(128/2,30,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Check if red dot is preset");
    this->u8g2->drawButtonUTF8(128/2,40,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "If not, something may be blocking laser");
    this->u8g2->drawButtonUTF8(128/2,50,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "blocking laser.");
    this->u8g2->drawButtonUTF8(128/2,60,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Hold back to quit");


    this->u8g2->sendBuffer();


}

void Menu::deactivateGuideLaser(){
    requestCommand = DEACTIVATE_GUIDE_LASER;
    this->u8g2->clearBuffer();
    this->u8g2->drawButtonUTF8(105, 10, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");
    this->u8g2->drawButtonUTF8(128/2,20,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Toggling Guide Laser");
    this->u8g2->sendBuffer();
}

void Menu::homeAll(){

    this->encoder->setPosition(0); // Always on back

    requestCommand = HOME_ALL;
    this->u8g2->clearBuffer();
    this->u8g2->drawButtonUTF8(105, 10, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");
    this->u8g2->drawButtonUTF8(128/2, 20, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Homing all");
    this->u8g2->drawButtonUTF8(128/2,30,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Components");
    this->u8g2->sendBuffer();
}

void Menu::homePush(){

    this->encoder->setPosition(0); // Always on back

    requestCommand = HOME_PUSHBAR;
    this->u8g2->clearBuffer();
    this->u8g2->drawButtonUTF8(105, 10, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");
    this->u8g2->drawButtonUTF8(128/2, 20, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Homing ");
    this->u8g2->drawButtonUTF8(128/2,30,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Pushbar");
    this->u8g2->sendBuffer();

}

void Menu::homePlates(){

    this->encoder->setPosition(0); // Always on back

    requestCommand = HOME_PLATES;
    this->u8g2->clearBuffer();
    this->u8g2->drawButtonUTF8(105, 10, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");
    this->u8g2->drawButtonUTF8(128/2, 20, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Homing ");
    this->u8g2->drawButtonUTF8(128/2,30,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Build & Load Plate");
    this->u8g2->sendBuffer();
}

void Menu::prepPrint(){

    this->encoder->setPosition(0); // Always on back

    requestCommand = PREP_PRINT;
    this->u8g2->clearBuffer();
    this->u8g2->drawButtonUTF8(105, 10, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");
    this->u8g2->drawButtonUTF8(128/2, 20, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Lifting Build");
    this->u8g2->drawButtonUTF8(128/2,30,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Plate");
    this->u8g2->sendBuffer();
}

void Menu::movePushBar(){
    this->encoder->setPosition(0); // Always on back

    requestCommand = MOVE_BAR;
    this->u8g2->clearBuffer();
    this->u8g2->drawButtonUTF8(105, 10, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");
    this->u8g2->drawButtonUTF8(128/2, 20, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Testing");
    this->u8g2->drawButtonUTF8(128/2,30,(encoder->getPosition() == 1 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Pushbar");
    this->u8g2->sendBuffer();
}

void Menu::printCube(){
//this->encoder->setPosition(1); // Always on back

        
    

    this->u8g2->clearBuffer();
    //this->u8g2->drawButtonUTF8(105, 10, (U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");

    if(waitforResponse == 0){
        requestCommand = PRINT_CUBE;
        this->u8g2->drawButtonUTF8(128/2,20,( U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Prepping Print");
        this->u8g2->drawButtonUTF8(128/2,30,( U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Please wait...");

    }
    else if ( waitforResponse == 1){
        
        this->u8g2->drawButtonUTF8(128/2,20,(U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Printer Ready!");
        this->u8g2->drawButtonUTF8(128/2,30,( U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Please insert material");
        this->u8g2->drawButtonUTF8(128/2,40,( U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "and degass container");
        this->u8g2->drawButtonUTF8(128/2,50,( U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Press encoder to cont.");
    }
    else if(waitforResponse == 2){
        this->u8g2->drawButtonUTF8(128/2,20,(U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Print Active!");
        this->u8g2->drawButtonUTF8(128/2,30,( U8G2_BTN_HCENTER|U8G2_BTN_INV), 34 ,2 ,0, "Fiber Laser Active");
    }

    this->u8g2->sendBuffer();
}

void Menu::refreshGcodeFiles() {
    if (!sdReady) {
        sdReady = sdGcode.begin();
    }

    gcodeFiles.clear();
    if (sdReady) {
        gcodeFiles = sdGcode.getGcodeFiles("/");
    }

    gcodeFilesLoaded = true;
}

void Menu::printGcodeFiles() {
    if (!gcodeFilesLoaded) {
        refreshGcodeFiles();
    }

    this->u8g2->clearBuffer();
    this->u8g2->setFont(u8g2_font_helvR08_tf);
    this->u8g2->drawStr(0, 10, "SD Gcode Files");
    this->u8g2->drawLine(0, 12, 95, 12);

    const int totalItems = static_cast<int>(gcodeFiles.size()) + 1; // Back + files
    this->encoder->setMinMax(0, totalItems - 1);

    int selected = this->encoder->getPosition();
    const int visibleRows = 4;

    if (selected < visibleTop) {
        visibleTop = selected;
    }
    if (selected >= visibleTop + visibleRows) {
        visibleTop = selected - visibleRows + 1;
    }

    for (int row = 0; row < visibleRows; row++) {
        int itemIndex = visibleTop + row;
        if (itemIndex >= totalItems) {
            break;
        }

        String label = (itemIndex == 0) ? "Back" : compactFileName(gcodeFiles[itemIndex - 1]);
        int y = 10 + (row + 1) * 12;

        this->u8g2->drawButtonUTF8(
            128 / 2,
            y,
            (selected == itemIndex ? U8G2_BTN_HCENTER | U8G2_BTN_BW2 : U8G2_BTN_HCENTER | U8G2_BTN_INV),
            34,
            2,
            0,
            label.c_str());
    }

    if (!sdReady) {
        this->u8g2->drawStr(0, 62, "SD init failed");
    } else if (gcodeFiles.empty()) {
        this->u8g2->drawStr(0, 62, "No .gcode files found");
    }

    this->u8g2->sendBuffer();
}

void Menu::printGcodeStatus() {
    this->u8g2->clearBuffer();
    this->u8g2->setFont(u8g2_font_helvR08_tf);
    this->u8g2->drawStr(0, 10, "Gcode Print Active");
    this->u8g2->drawLine(0, 12, 110, 12);

    char progress[24];
    int percentage = (gcodeTotalLines > 0) ? (100 * gcodeCurrentLine) / gcodeTotalLines : 0;
    snprintf(progress, sizeof(progress), "%d/%d %d%%", gcodeCurrentLine, gcodeTotalLines, percentage);

    this->u8g2->drawStr(0, 24, "Progress:");
    this->u8g2->drawStr(54, 24, progress);
    this->u8g2->drawStr(0, 36, "Line:");

    String compactLine = compactFileName(gcodeCurrentLineText, 20);
    this->u8g2->drawStr(30, 36, compactLine.c_str());

    const char* pauseLabel = gcodePrintPaused ? "Resume" : "Pause";

    this->u8g2->drawButtonUTF8(128 / 2, 50,
        (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER | U8G2_BTN_BW2 : U8G2_BTN_HCENTER | U8G2_BTN_INV),
        34, 2, 0, pauseLabel);

    this->u8g2->drawButtonUTF8(128 / 2, 62,
        (encoder->getPosition() == 1 ? U8G2_BTN_HCENTER | U8G2_BTN_BW2 : U8G2_BTN_HCENTER | U8G2_BTN_INV),
        34, 2, 0, "Cancel");

    this->u8g2->sendBuffer();
}

//--------------------------------------------------------------

void Menu::goToMainMenu() {
    if (rootNode != nullptr) {
        currentNode = rootNode;
    }
    currentSelection = 0;
    visibleTop = 0;
    encoder->setPosition(0);
}

void Menu::back(){
    if (gcodePrintActive) {
        return;
    }

    if(this->currentNode->back != nullptr){
        requestCommand = 0;
        this->currentNode = this->currentNode->back;
    }
}

void Menu::printChildren(Tnode* node, char* name, bool printLayerChildren = true, bool sendBuffer = true){

    this->u8g2->clearBuffer();
    this->u8g2->setFont(u8g2_font_helvR08_tf); // Set any readable font

    this->u8g2->drawStr(0, 10, name);
    this->u8g2->drawLine(0, 12, 75, 12);
    this->encoder->setMinMax(0, node->children.size());
    //this->encoder->setPosition(0);
    this->u8g2->drawButtonUTF8(105, 10, (encoder->getPosition() == 0 ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV), 34, 2, 0, "Back");

    if(printLayerChildren){
        std::vector<String> children = node->getChildrenNames();

        for(int i = 0; i < children.size(); i++){

            this->u8g2->drawButtonUTF8(128/2, 10 + (i+1)*12, (encoder->getPosition() == (i+1) ? U8G2_BTN_HCENTER|U8G2_BTN_BW2 : U8G2_BTN_HCENTER|U8G2_BTN_INV)  , 34 , 2 ,0, children[i].c_str());
        } 
    }
    
    
    if(sendBuffer) this->u8g2->sendBuffer();
}
