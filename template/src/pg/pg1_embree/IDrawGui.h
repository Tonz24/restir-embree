#pragma once

class IDrawGui {
public:
	virtual ~IDrawGui() = default;
	virtual void drawGui() = 0;
};