#define TX_JERRY_IMPL
#include "libs/TXLib/txlib.hpp"
#include "libs/TXLib/txgraphics.hpp"
#include "libs/TXLib/txutility.hpp"
#include "libs/TXLib/txmath.hpp"
#include "libs/TXLib/txmap.hpp"
#include "libs/TXLib/txjson.hpp"

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



class HilbertCurve {
public:



	void drawHilbert(int iterationCount) {
		this->gridSize = 1.0f / std::powf(2.0f, iterationCount); // 2.0f / std::powf(2.0f, (iterationCount + 1))
		this->halfGridSize = gridSize * 0.5f;

		drawHilbert_impl(iterationCount, trans_impl{});
		this->current = tx::CoordOrigin;
	}








private:
	static constexpr tx::Coord baseMovement[3] = {
		{ 0,  1 },
		{ 1,  0 },
		{ 0, -1 }
	};

	class transformation_impl {
	public:

		void applyLeftTrans () { inverse = !inverse; }
		void applyRightTrans() { inverse = !inverse; sign = -sign; }
		tx::Coord applyToCoord(const tx::Coord& in) const {
			return tx::Coord{
				(inverse ? in.y() : in.x()) * sign,
				(inverse ? in.x() : in.y()) * sign
			};
		}

		transformation_impl operator*(transformation_impl other) const { return transformation_impl{
			static_cast<bool>(this->inverse ^ other.inverse),
			static_cast<char>(this->sign * other.sign)}; }

		bool inverse = 0;
		char sign = 1;
	};
	using trans_impl = transformation_impl;

	static constexpr trans_impl sectionDefaultTrans[4] = {
		{ (bool)1, (char) 1 },
		{       }, {        },
		{ (bool)1, (char)-1 }
	};

	


	tx::Coord current = tx::CoordOrigin;
	float gridSize,
		halfGridSize;


	void updateCurrent_impl(const tx::Coord& increment) {
		tx::Coord temp = current + increment;
		drawLine_impl(current, temp);
		current = temp;
	}
	// the only actual drawing function out of all these functions that have the name drawXxx, how ironic
	void drawLine_impl(const tx::Coord& a, const tx::Coord& b) const {
		tx::drawLine(toScreenCoord_impl(a), toScreenCoord_impl(b));
	}
	tx::vec2 toScreenCoord_impl(const tx::Coord& in) const {
		return (in * gridSize).offset(halfGridSize - 1.0f, halfGridSize - 1.0f);
	}

	// recursive
	void drawHilbert_impl(int iterationCount, trans_impl trans) {
		if (!iterationCount) {
			for (int i = 0; i < 3; i++) { // 3 is not a magic number, it's the number of magic.... did you laugh? it's acutally the number of lines in a smallest pattern of Hilbert Curve
				updateCurrent_impl(trans.applyToCoord(baseMovement[i]));
			}
		}
		else {
			for (int i = 0; i < 4; i++) { // 4 is also not a magic number, it's a number full of magic.... sorry for the bad joke again, it's actually the number of sections of the smallest pattern of Hilbert Curve
				drawHilbert_impl(iterationCount - 1, sectionDefaultTrans[i] * trans);
				drawConnectLine_impl(i, trans);
			}
		}
	}

	void drawConnectLine_impl(int index, trans_impl trans) {
		if (index >= 3) return;
		updateCurrent_impl(trans.applyToCoord(baseMovement[index]));
	}











};













































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