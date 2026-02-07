#define TX_JERRY_IMPL
#include "TXLib/txlib.hpp"
#include "TXLib/txgraphics.hpp"
#include "TXLib/txutility.hpp"
#include "TXLib/txmath.hpp"
#include "TXLib/txmap.hpp"
#include "TXLib/txjson.hpp"

void drawMathLine(const tx::MathLine& line) {
	tx::drawLine(tx::vec2{ -1.0f, tx::findLineY(line, -1.0f) }, tx::vec2{ 1.0f, tx::findLineY(line, 1.0f) });
}
tx::JsonObject initJsonObject(const std::filesystem::path& filePath) {
	string str;
	tx::readWholeFile(filePath, str);

	tx::JsonObject root = tx::JsonObject{};
	tx::JsonParser parser;
	parser.parse(str, root);
}











































// UI

class DragWidget {
public:
	DragWidget(const tx::vec2& in_pos, float in_radius, tx::vec2* in_updatePos, std::function<void(const tx::vec2&)> in_eventCb) :
		position(in_pos), radius(in_radius), updatePos(in_updatePos), eventCb(in_eventCb)
	{


	}



	void update(const tx::vec2& cursorPos, bool keyDown_mouseLeft) {
		if (inProcess_drag) {
			if (!keyDown_mouseLeft) {
				inProcess_drag = 0;
			}
			else {
				// call event callback
				this->updatePos->operator+=(cursorPos - position);
				this->position = cursorPos;
				this->eventCb(cursorPos);
			}
		}
		else {
			if (inProcess_mouseHover) {
				if (!inRange(cursorPos, position, radius)) {
					inProcess_mouseHover = 0;
					// call exit func
				}
				else if (keyDown_mouseLeft) {
					inProcess_drag = 1;
				}				
			}
			else {
				if (inRange(cursorPos, position, radius)) {
					inProcess_mouseHover = 1;
					// call enter func
				}
			}			
		}
	}
	void draw() {
		tx::drawCircle(position, radius);




	}


private:
	tx::vec2 position;
	float radius;
	tx::vec2* updatePos;
	std::function<void(const tx::vec2&)> eventCb;
	

	bool inProcess_mouseHover = 0;
	bool inProcess_drag = 0;

	bool inRange(const tx::vec2& pos, const tx::vec2 domainPos, float domainRadius) {
		tx::vec2 normalVec = pos - domainPos;
		float distSq = tx::sq(normalVec.x()) + tx::sq(normalVec.y());
		float radiusSq = tx::sq(domainRadius);
		return distSq <= radiusSq;
	}







};