#include "Tenshi/GUI/Manager.h"

#include "Tenshi/GUI/Element.h"
#include "Tenshi/Core/Game.h"

namespace Tenshi {

UIManager::UIManager(Game* game) : Subroutine(game) {
}

UIManager::~UIManager() {
}

void UIManager::event(const Event& e) {
    for (auto& child : children) {
        child->event(e);
    }
}

void UIManager::update(double deltaTime) {
    for (auto& child : children) {
        child->update(deltaTime);
    }
}

void UIManager::fixedUpdate(double deltaTime) {
    // no need to do anything
}

void UIManager::draw() {
    for (auto& child : children) {
        child->draw(game->getRenderer());
    }
}

void UIManager::pushChild(std::unique_ptr<GUI::Element> child) {
    children.push_back(std::move(child));
}

} // namespace Tenshi