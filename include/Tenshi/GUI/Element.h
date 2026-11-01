#pragma once

#include "Tenshi/Utility/Types.h"
#include "Tenshi/Event/Event.h"
#include "Tenshi/Graphics/Renderer.h"

#include <vector>
#include <memory>
#include <string>
#include <algorithm>

namespace Tenshi {
namespace GUI {

class Element {
public:
    Element(std::string id, Rect<float> bounds)
    :   id(id)
    ,   bounds(bounds)
    {}

    virtual ~Element() = default;

    virtual bool event(const Event& event) {
        if (!active) return false;

        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            if ((*it)->event(event)) {
                return true;
            }
        }

        return false;
    }

    virtual void update(float deltaTime) {
        if (!active) return;

        for (auto& child : children) {
            child->update(deltaTime);
        }
    }
    
    virtual void draw(Renderer& renderer) {
        if (!active) return;

        // draw self...
        
        for (auto& child : children) {
            child->draw(renderer);
        }
    }

    void addChild(std::unique_ptr<Element> child) {
        if (!child) return;
        child->parent = this;
        children.push_back(std::move(child));
    }

    bool removeChild(const std::string& id) {
        auto it = std::remove_if(children.begin(), children.end(),
            [&id](const std::unique_ptr<Element>& child) {
                return child->id == id;
            });
        
        if (it != children.end()) {
            children.erase(it, children.end());
            return true;
        }
        return false;
    }

    Vec<float, 2> getGlobalPosition() const {
        if (parent) {
            Vec<float, 2> parentGlobal = parent->getGlobalPosition();
            return { parentGlobal.x + bounds.x, parentGlobal.y + bounds.y };
        }
        return Vec<float, 2>{bounds.x, bounds.y};
    }

    void setActive(bool active) { active = active; }

    bool isActive() const { return active; }
    const std::string& getId() const { return id; }

protected:
    std::string id;
    Element* parent = nullptr;
    std::vector<std::unique_ptr<Element>> children;

    Rect<float> bounds;
    bool active = true;
};

} // namespace GUI
} // namespace Tenshi