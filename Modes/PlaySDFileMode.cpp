#include "PlaySDFileMode.h"
PlaySDFileMode::PlaySDFileMode(Laser& laser) : _laser(laser) {

}

void PlaySDFileMode::execute() {
    
}

void PlaySDFileMode::stop() {

}

LaserMode PlaySDFileMode::getModeName() {
    return LaserMode::SDCardMode;
}