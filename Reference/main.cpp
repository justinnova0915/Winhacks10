#include "header.h"
#include "libs/GLFW/include/glfw3.h"

//#include "ZigZag.h"
//#include "Bad Apple!!.h";

#define PI 3.1415926f
#define ONE_DEGREE 0.017453292f



#define FIXED_TICKRATE 60.0
#ifdef FIXED_TICKRATE
double tickIntervalTime = 1.0 / FIXED_TICKRATE; // seconds
double maxAccumulatorTime = tickIntervalTime * 5;
#endif

//#define USING_BGM
#ifdef USING_BGM
#define MINIAUDIO_IMPLEMENTATION
#include "libs/miniaudio/miniaudio.h"
#endif


template<class Enum, class = std::enable_if_t<std::is_enum_v<Enum>>>
inline Enum operator|(Enum a, Enum b) {
	using T = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<T>(a) | static_cast<T>(b));
}
template<class Enum, class = std::enable_if_t<std::is_enum_v<Enum>>>
inline Enum& operator|=(Enum& a, Enum b) {
	using T = std::underlying_type_t<Enum>;
	return (a = static_cast<Enum>(static_cast<T>(a) | static_cast<T>(b)));
}
template<class Enum, class = std::enable_if_t<std::is_enum_v<Enum>>>
inline bool operator&(Enum a, Enum b) {
	using T = std::underlying_type_t<Enum>;
	return (static_cast<T>(a) & static_cast<T>(b));
}

std::filesystem::path getExePath() {
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	return std::filesystem::path(buffer);
}
enum TimerFlag {
	NoEndLog,
	AutoLog
};
class Timer {
public:
	Timer(string in_name = "No_Name", TimerFlag in_properties = AutoLog) : name(in_name), properties(in_properties), start(std::chrono::steady_clock::now()) {}
	~Timer() {
		if (this->properties & AutoLog) {
			auto end = std::chrono::steady_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			this->ofs << this->name << ": " << duration.count() << '\n';
			this->ofs.close();
		}
	}
	int duration() {
		auto end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		return duration.count();

		//return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	}
	long long durationL() {
		auto end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
		return duration.count();
	}
private:
	string name;

	int properties;

	static std::ofstream ofs;
	std::chrono::steady_clock::time_point start;
};
std::ofstream Timer::ofs = std::ofstream(getExePath().parent_path() / "debugLog.txt", std::ios::app);
void debugLogInit() {
	//std::filesystem::remove(getExePath().parent_path() / "debugLog.txt");
}


class Log {
public:
	template<class T>
	Log& operator<<(const T in) {
		if (!Log::isActive) return *this;
		this->oss << in;
		return *this;
	}
	Log& operator<<(void(*func)(Log* obj)) {
		if (!Log::isActive) return *this;
		func(this);
		return *this;
	}
	static void print(Log* obj) {
		std::lock_guard<std::mutex> lock(Log::mutex);
		cout << obj->oss.str() << Color::Reset << '\n';
		obj->oss.str("");
	}
	class Color {
	public:
		static constexpr const char* Reset = "\033[0m";
		static constexpr const char* Bold = "\033[1m";
		static constexpr const char* UnderLine = "\033[4m";

		static constexpr const char* Black = "\033[30m";
		static constexpr const char* Red = "\033[31m";
		static constexpr const char* Green = "\033[32m";
		static constexpr const char* Yellow = "\033[33m";
		static constexpr const char* Blue = "\033[34m";
		static constexpr const char* Magenta = "\033[35m";
		static constexpr const char* Cyan = "\033[36m";
		static constexpr const char* White = "\033[37m";

		static constexpr const char* BrightBlack = "\033[90m";
		static constexpr const char* BrightRed = "\033[91m";
		static constexpr const char* BrightGreen = "\033[92m";
		static constexpr const char* BrightYellow = "\033[93m";
		static constexpr const char* BrightBlue = "\033[94m";
		static constexpr const char* BrightMagenta = "\033[95m";
		static constexpr const char* BrightCyan = "\033[96m";
		static constexpr const char* BrightWhite = "\033[97m";
	};
private:
	std::ostringstream oss;
	static std::mutex mutex;
	static bool isActive;
};
std::mutex Log::mutex;
bool Log::isActive = 0;

class LogFile {
public:
	template<class T>
	LogFile& operator<<(const T in) {
		if (!LogFile::isActive) return *this;
		this->oss << in;
		return *this;
	}
	LogFile& operator<<(void(*func)(LogFile* obj)) {
		if (!LogFile::isActive) return *this;
		func(this);
		return *this;
	}
	static void print(LogFile* obj) {
		std::lock_guard<std::mutex> lock(LogFile::mutex);
		LogFile::ofs << obj->oss.str() << '\n';
		LogFile::ofs.flush();
		obj->oss.str("");
	}
	
private:
	std::ostringstream oss;
	static std::ofstream ofs;
	static std::mutex mutex;
	static bool isActive;
};

std::mutex LogFile::mutex;
bool LogFile::isActive = 1;
std::ofstream LogFile::ofs = std::ofstream(getExePath().parent_path() / "debugLog.txt");


GLFWwindow* windowG;

// function from function.h
//void clearConsole() {
//	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//	CONSOLE_SCREEN_BUFFER_INFO csbi;
//	GetConsoleScreenBufferInfo(hConsole, &csbi);
//	DWORD consoleSize = csbi.dwSize.X * csbi.dwSize.Y;
//	DWORD charsWritten;
//	COORD topLeft = { 0, 0 };
//	FillConsoleOutputCharacter(hConsole, ' ', consoleSize, topLeft, &charsWritten);
//	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, consoleSize, topLeft, &charsWritten);
//	SetConsoleCursorPosition(hConsole, topLeft);
//}
//std::filesystem::path getExePath() {
//	char buffer[MAX_PATH];
//	GetModuleFileNameA(NULL, buffer, MAX_PATH);
//	return std::filesystem::path(buffer);
//}
// vec2 ***********************************************************************************************************************
class vec2;
inline float dot(vec2 in1, vec2 in2);
class vec2 {
public:
	vec2(float in_x, float in_y) :
		m_x(in_x), m_y(in_y) {

	}
	vec2() : m_x(0.0f), m_y(0.0f) {}
	inline void setX(float in_x) { this->m_x = in_x; }
	inline void setY(float in_y) { this->m_y = in_y; }
	inline float getX() const { return this->m_x; }
	inline float x() const { return this->m_x; }
	inline float getY() const { return this->m_y; }
	inline float y() const { return this->m_y; }

	vec2 operator+(const vec2& other) const { return vec2(this->m_x + other.m_x, this->m_y + other.m_y); }
	vec2 operator-(const vec2& other) const { return vec2(this->m_x - other.m_x, this->m_y - other.m_y); }
	vec2 operator*(const vec2& other) const { return vec2(this->m_x * other.m_x, this->m_y * other.m_y); }
	vec2 operator*(float coef) const { return vec2(this->m_x * coef, this->m_y * coef); }
	vec2 operator+=(const vec2& other) { this->m_x += other.m_x; this->m_y += other.m_y; return *this; }
	vec2 operator-=(const vec2& other) { this->m_x -= other.m_x; this->m_y -= other.m_y; return *this; }
	vec2 operator*=(const vec2& other) { this->m_x *= other.m_x; this->m_y *= other.m_y; return *this; }
	vec2 operator*=(float coef) { this->m_x *= coef; this->m_y *= coef; return *this; }
	//vec2 operator/(const vec2& other) { return vec2(this->m_x / other.m_x, this->m_y / other.m_y); }

	vec2 operator=(const vec2& other) { this->m_x = other.m_x; this->m_y = other.m_y; return other; }
	vec2 operator=(float other) { this->m_x = other; this->m_y = other; return *this; }
	bool operator==(const vec2& other) const { return this->m_x == other.m_x && this->m_y == other.m_y; }
	bool operator!=(const vec2& other) const { return !(this->m_x == other.m_x && this->m_y == other.m_y); }
	bool operator<(const vec2& other) const { return this->m_x * this->m_x + this->m_y * this->m_y < other.m_x * other.m_x + other.m_y * other.m_y; }
	bool operator>(const vec2& other) const { return this->m_x * this->m_x + this->m_y * this->m_y > other.m_x * other.m_x + other.m_y * other.m_y; }

	inline float length() {
		return std::sqrtf(dot(*this, *this));
	}

private:
	float m_x, m_y;
};
vec2 operator*(float coef, vec2 vec) { return vec2(vec.getX() * coef, vec.getY() * coef); }

// Math ***********************************************************************************************************************
inline float sign(float num) { return (num / std::fabs(num)); }
inline float dot(vec2 in1, vec2 in2) { return (in1.getX() * in2.getX() + in1.getY() * in2.getY()); }
inline float sq(float in) { return in * in; }
inline int sq(int in) { return in * in; }
inline float hypot(vec2 in) { return dot(in, in); }
inline vec2 unify(vec2 in, float* len = nullptr) {
	float scalar = std::sqrtf(dot(in, in));
	if (scalar == 0.0f) return vec2(0, 0);
	if (len != nullptr) *len = scalar;
	return vec2(in.getX() / scalar, in.getY() / scalar);
}
#undef min
#undef max
template<class T>
inline T min(T a, T b) { return a < b ? a : b; }
template<class T>
inline T max(T a, T b) { return a > b ? a : b; }

class Coord {
public:
	Coord(int in_x, int in_y) :
		m_x(in_x), m_y(in_y) {}
	Coord() : m_x(0.0f), m_y(0.0f) {}
	inline void setX(int in_x) { this->m_x = in_x; }
	inline void setY(int in_y) { this->m_y = in_y; }
	inline int getX() const { return this->m_x; }
	inline int x() const { return this->m_x; }
	inline int getY() const { return this->m_y; }
	inline int y() const { return this->m_y; }

	Coord operator+(const Coord& other) const { return Coord(this->m_x + other.m_x, this->m_y + other.m_y); }
	Coord operator-(const Coord& other) const { return Coord(this->m_x - other.m_x, this->m_y - other.m_y); }
	Coord operator+=(const Coord& other) { this->m_x += other.m_x; this->m_y += other.m_y; return *this; }
	Coord operator-=(const Coord& other) { this->m_x -= other.m_x; this->m_y -= other.m_y; return *this; }


	Coord operator=(const Coord& other) { this->m_x = other.m_x; this->m_y = other.m_y; return other; }
	bool operator==(const Coord& other) const { return this->m_x == other.m_x && this->m_y == other.m_y; }
	bool operator!=(const Coord& other) const { return !(this->m_x == other.m_x && this->m_y == other.m_y); }
	bool operator<(const Coord& other) const { return this->m_x * this->m_x + this->m_y * this->m_y < other.m_x * other.m_x + other.m_y * other.m_y; }
	bool operator>(const Coord& other) const { return this->m_x * this->m_x + this->m_y * this->m_y > other.m_x * other.m_x + other.m_y * other.m_y; }

	inline bool valid(int edge) {
		return m_x >= 0 && m_y >= 0 && m_x < edge && m_y < edge;
	}

private:
	int m_x, m_y;
};
// seed wound be ther final hashed value, and value would be the inputing value
inline void hashCombine(std::size_t& seed, std::size_t value) {
	seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}
class coordHash {
public:
	size_t operator()(const Coord& in) const {
		size_t seed = 0;
		hashCombine(seed, in.getX());
		hashCombine(seed, in.getY());
		return seed;
	}
};

#undef RGB
class RGB {
public:
	RGB(float in_r, float in_g, float in_b) :
		m_r(in_r), m_g(in_g), m_b(in_b)
	{



	}

	inline const float r() const { return this->m_r; }
	inline const float g() const { return this->m_g; }
	inline const float b() const { return this->m_b; }



private:
	float m_r, m_g, m_b;
};

class Rainbow {
public:
		Rainbow(int range) {
			float increment = PI / range,
				third = PI / 3;
			for (int i = 0; i < range; i++) {
				m_rainbow.push_back(RGB(
					std::abs(std::sinf(increment * i)),
					std::abs(std::sinf(increment * i + third)),
					std::abs(std::sinf(increment * i + third * 2))
				));
			}




		}

		inline RGB operator[](int index) const {
			return m_rainbow[index];
		}
		inline RGB getNextColor() {
			index++;
			if (index == this->m_rainbow.size()) {
				index = 0;
			} return m_rainbow[index];
		}



private:

	vector<RGB> m_rainbow;
	int index = -1;


};







void drawSquare(float x, float y, float w, float h) {
	glVertex2f(x, y);
	glVertex2f(x + w, y);
	glVertex2f(x, y + h);

	glVertex2f(x + w, y);
	glVertex2f(x, y + h);
	glVertex2f(x + w, y + h);
}
vector<vec2> initCircleVertexs() {
	vector<vec2> pos;
	float degree = 0.0, sideAmount = 6.0, degreeIncrement = 360.0 / sideAmount * ONE_DEGREE;
	while (degree <= 2 * PI) {
		pos.push_back({ std::cosf(degree), std::sinf(degree) });
		degree += degreeIncrement;
	} return pos;
}
void drawCircle(float x, float y, float radius) {
	static vector<vec2> pos = initCircleVertexs(); // { { x, y } }
	vec2 now(x + pos[0].x() * radius, y + pos[0].y() * radius);
	for (int i = 0; i < pos.size(); i++) {
		int next = i + 1 == pos.size() ? 0 : i + 1;
		glVertex2f(x, y);
		glVertex2f(now.x(), now.y());
		now.setX(x + pos[next].x() * radius); now.setY(y + pos[next].y() * radius);
		glVertex2f(now.x(), now.y());
	}
}
void drawCircle(const vec2& pos, float radius) {
	drawCircle(pos.x(), pos.y(), radius);
}








class GravitySys;
enum class EntityType : uint8_t {
	Gravity      = 0b00000001,
	Floating     = 0b00000010,
	Immune       = 0b00000100,
	Falling      = 0b00001000,
	Static       = 0b00010000,
	GravityPoint = 0b00100000
};
class PointObject {
public:
	PointObject(vec2 in_pos) :
		position(in_pos)
	{}

	inline void setX(float in_x) { this->position.setX(in_x); }
	inline void setY(float in_y) { this->position.setY(in_y); }
	inline void setPos(const vec2& in) { this->position = in; }
	inline void setPosition(const vec2& in) { this->position = in; }


	inline float getX() const { return this->position.x(); }
	inline float getY() const { return this->position.y(); }
	inline const vec2& getPos() const { return this->position; }
	inline const vec2& getPosition() const { return this->position; }

protected:

	vec2 position;

};
class PhysicalObject : public PointObject {
	friend class GravitySys;
public:
	PhysicalObject(float in_x, float in_y, float in_r, float in_xVelocity = 0.0f, float in_yVelocity = 0.0f, float in_constantAccelerationX = 0.0f, float in_constantAccelerationY = 0.0f) :
		PointObject(vec2(in_x, in_y)), lastPosition(in_x - in_xVelocity, in_y - in_yVelocity), radius(in_r), constantAcceleration(in_constantAccelerationX, in_constantAccelerationY) {
	
	}

	PhysicalObject(const vec2& in_pos, float in_r, const vec2& in_velocity = { 0.0f, 0.0f }, const vec2& in_constantAcceleration = { 0.0f, 0.0f }) :
		PointObject(in_pos), lastPosition(in_pos - in_velocity), radius(in_r), constantAcceleration(in_constantAcceleration) {
	}
	void update() {
		vec2 temp = this->position;
		/*vec2 velocity = this->lastPosition - this->position;
		vec2 friction(0.0f, 0.0f);
		if (hypot(velocity) > PhysicalObject::Friction) {
			friction = unify(velocity) * PhysicalObject::Friction;
		}*/
		this->position += (this->position - this->lastPosition) * PhysicalObject::Friction + this->acceleration;
		this->lastPosition = temp;
		this->acceleration = constantAcceleration;
	}

	
	inline void setR(float in_r) { this->radius = in_r; }
	inline float getR() const { return this->radius; }	
	
	const vec2& getVelocity() const { return (position - lastPosition); }
	
	inline void accelerateX(float in) { this->acceleration.setX(this->acceleration.x() + in); }
	inline void accelerateY(float in) { this->acceleration.setY(this->acceleration.y() + in); }
	inline void accelerate(const vec2& in) { this->acceleration += in; }

	inline static void setFriction(float in) { PhysicalObject::Friction = in; }


	inline const float getlpx() const { return this->lastPosition.x(); }
	inline const float getlpy() const { return this->lastPosition.y(); }

	/*inline void reverseXVelocity() {
		float temp = this->lastPosition.getX();
		this->lastPosition.setX(this->position.getX());
		this->position.setX(temp);
	}
	inline void reverseYVelocity() {
		float temp = this->lastPosition.getY();
		this->lastPosition.setY(this->position.getY());
		this->position.setY(temp);
	}*/


private:
	vec2 acceleration = { 0, 0 },
		lastPosition = { 0, 0 },
		preservedMovement = { 0, 0 },
		constantAcceleration = { 0, 0 };
	Coord gridPos = { -1, -1 };
	float radius;
	int gridIndex = -1;
	bool havePreservedMovement = 0;
	
	static float Friction;
	
	inline void setLastPosition(const vec2& in) { this->lastPosition = in; }
	inline void setlpx(float in) { this->lastPosition.setX(in); }
	inline void setlpy(float in) { this->lastPosition.setY(in); }
	inline const vec2& getLastPosition() const { return this->lastPosition; }/*
	inline const float getlpx() const { return this->lastPosition.x(); }
	inline const float getlpy() const { return this->lastPosition.y(); }*/

	inline void addPreservedMovement(const vec2& in) { this->preservedMovement += in; this->havePreservedMovement = 1; }
	void applyPreservedMovement() {
		if (this->havePreservedMovement) {
			this->position += this->preservedMovement;
			this->lastPosition += this->preservedMovement; // to keep velocity the same
			this->havePreservedMovement = 0;
			this->preservedMovement = { 0, 0 };
		}
	}

	//
	//inline void setVelocityX(float in) { this->xVelocity = in; }
	//inline void setVelocityY(float in) { this->yVelocity = in; }
	//inline void setVelocity(const vec2& in) { this->xVelocity = in.getX(); this->yVelocity = in.getY(); }
	//
	





};
float PhysicalObject::Friction = 0.0f;
//vector<PhysicalObject> emptyPhysicsEntityVec;
vector<PhysicalObject*> entitiesG;
void displayImmidietly() {
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	for (const PhysicalObject* i : entitiesG) {
		drawCircle(i->getX(), i->getY(), i->getR());
	}
	glEnd();
	glfwSwapBuffers(windowG);
}







// Engine *********************************************************************************************************************
// 整个物理引擎架构就是一个奇怪的三角恋。
// 我爱 Euler，Euler 不爱我。
// Verlet 爱我，我不爱 Verlet。
// Verlet -> 我 -> Euler

constexpr int GridMaxOccupation = 51;
constexpr int GridSizeIndex = GridMaxOccupation - 1;
class GravitySys {
public:
	GravitySys(
		int in_EntityMaxAmount = 100000,
		int in_CollisionUpdateMaxTimes = 8, 
		int in_substep = 8,
		float in_Gravity = 0.0003f,
		float in_Restitution = 0.9f,
		float in_Slop = 0.00000f, 
		float in_Friction = 0.95f,
		float in_OverlapAdjustPercentage = 0.5f) : 
		EntityMaxAmount(in_EntityMaxAmount),
		CollisionUpdateMaxTimes(in_CollisionUpdateMaxTimes),
		Substep(in_substep),
		Gravity(in_Gravity / in_substep),
		Restitution(in_Restitution),
		Slop(in_Slop),
		Friction(in_Friction),
		OverlapAdjustPercentage(in_OverlapAdjustPercentage)
	{
		this->entities.reserve(this->EntityMaxAmount);
		this->points.reserve(this->EntityMaxAmount);
		//PhysicalObject::setFriction(1.0);
		PhysicalObject::setFriction(this->Friction);
		

		
		//// init random index for sdjusting position traverse
		//vector<int> indexTemplate(this->entityMaxAmount, 0);
		//for (int i = 0; i < indexTemplate.size(); i++) {
		//	indexTemplate[i] = i;
		//} this->rdIndex = vector<vector<int>>(10, indexTemplate);
		//for (vector<int>& i : this->rdIndex) {
		//	std::shuffle(i.begin(), i.end(), this->rdeIndex);
		//}

		


	}
	void update() {
		for (int i = 0; i < this->Substep; i++) {
			__update();
		}		
	}

	//void update() {
	//	// 1. Accumulate Forces
	//	applyGravity();

	//	// 2. Update Position (Verlet Integration)
	//	for (PhysicalObject& i : this->entities) {
	//		i.update();
	//	}

	//	// 3. Iterative Constraint Solving (Positional Correction + Explicit Bounce)
	//	// We run this multiple times to ensure stability and fully solve overlaps.
	//	for (int i = 0; i < this->collisionDetectionMaxTimes; i++) {
	//		solveCollisionsAndInjectBounce(this->entities);
	//		// solveBorderCollisions(this->entities); // Boundary checks can also be run iteratively
	//	}
	//}
	

	PhysicalObject* createPhysicalObject(PhysicalObject in_obj) {
		this->entities.push_back(in_obj);
		return &entities[entities.size() - 1];
	}
	PhysicalObject* createPhysicalObject(float in_x, float in_y, float in_r, float in_xVelocity, float in_yVelocity, EntityType in_status = EntityType::Gravity) {
		vec2 constantAcceleration;
		if (in_status & EntityType::Falling) constantAcceleration.setY(-this->Gravity);
		if (this->gridSysEnabled) in_r = this->fixedRadius;
		this->entities.push_back(PhysicalObject(in_x, in_y, in_r, in_xVelocity / this->Substep, in_yVelocity / this->Substep, constantAcceleration.getX(), constantAcceleration.getY()));
		if (in_status & EntityType::GravityPoint) this->gravityPoints.push_back(&entities[entities.size() - 1]);
		return &entities[entities.size() - 1];
	}
	
	PointObject* createGravityPoint(float in_x, float in_y) {
		this->points.push_back(vec2(in_x, in_y));
		this->gravityPoints.push_back(&this->points.back());
		return gravityPoints.back();
	}


	

	inline void enableGridSystem(float in_fixedRadius) {
		this->gridSysEnabled = 1;
		this->fixedRadius = in_fixedRadius;
		this->gridInit(in_fixedRadius);
	}
	void _gridUpdate() {
		gridUpdate();
	}
	inline void enableMultiThread(int in_threadAmount = 10) {
		this->threadAmount = in_threadAmount;
		threadInit();
	}






	//void enableFriciton(float in = 0.0001f) { this->Friction = in; }



private:
	vector<PhysicalObject> entities;
	vector<PointObject> points;
	vector<PointObject*> gravityPoints;
	// the last element indicates the next available index (aka end() iterator)
	vector<std::array<int, GridMaxOccupation>> Grid; // grid sys
	vector<std::array<std::array<int, GridMaxOccupation>*, 4>> gridAdjacent;
	vector<std::array<int, GridMaxOccupation>*> gridSide;
	vector<std::thread> threadPool; // threadPool
	std::queue<Coord> threadProcessQueue; // need optimization ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	vector<bool> threadBorderClearDeclaration;
	//std::mt19937 rdeIndex = std::mt19937(67);
	//std::uniform_int_distribution<int> distIndex = std::uniform_int_distribution<int>(0, 9);
	bool gridSysEnabled = 0;
	int EntityMaxAmount = 100000;
	int Substep = 8;
	float Gravity = 0.0003f, Restitution = 0.9f, Slop = 0.00000f, Friction = 0.95f, OverlapAdjustPercentage = 0.5f;
	int CollisionUpdateMaxTimes = 5;
	float fixedRadius = 0.0f;
	int gridRowAmount, gridSideLen; // grid sys var
	int threadAmount = 16; // thread pool var
	int threadSectionHeight = 5;
	int fullSectionAmount, fullSectionLength, remainingLength, totalSectionAmount;
	std::atomic<bool> threadNotTerminated = 1;
	std::atomic<int> threadCompletedCounter = 0;
	std::condition_variable cv_sendTask;
	std::condition_variable cv_completion;
	std::condition_variable cv_borderClear;
	std::mutex mx_worker;
	std::mutex mx_completion;
	std::mutex mx_borderClear;
	long long threadUpdateTime = 0; // for testing and recording

	std::atomic<int> maxGridObjectCount = 0;


	std::ofstream ofs = std::ofstream(getExePath().parent_path() / "debugLog.txt");

	void __update(){
		//Timer timer("Update");

		// accumulate force
		//applyFallingGravity();
		applyGravity();


		// update position
		for (PhysicalObject& i : this->entities) {
			i.update();
		}
		// update grid sys
		if (this->gridSysEnabled) {
			gridUpdate();
		}
		// update collison
		if (this->gridSysEnabled) {
			for (int i = 0; i < this->CollisionUpdateMaxTimes; i++) {
				updateCollision();
				solveBorderCollision();
				gridUpdate();
			}
		}
		else {
			for (int i = 0; i < this->CollisionUpdateMaxTimes; i++)
				solveCollision(this->entities);
		}




		//cout << this->maxGridObjectCount << endl;
		//this->maxGridObjectCount = 0;


	}





	// Multi Thread Sys (thread pool) *****************************************************************************************

	void threadInit() {
		this->threadBorderClearDeclaration.assign(this->totalSectionAmount, 0);
		threadBorderClearDeclaration.back() = 1;
		for (int i = 0; i < this->threadAmount; i++) {
			this->threadPool.emplace_back([this, i]() {
				this->threadWorker(i);
				});
		}
	}

	void threadWorker(int threadIndex) {
		while (this->threadNotTerminated) {
			Log log;
			Coord TaskInfo;

			{
				std::unique_lock<std::mutex> lock(this->mx_worker);
				this->cv_sendTask.wait(lock, [this]() {
					return !this->threadNotTerminated || !this->threadProcessQueue.empty();
					});

				if (!this->threadNotTerminated)
					return;

				TaskInfo = this->threadProcessQueue.front();
				this->threadProcessQueue.pop();

				log << "task received. Remaining tasks: " << this->threadProcessQueue.size() << Log::print;

			}

			//Timer timer("Thread");

			threadProcessSection(TaskInfo.x(), TaskInfo.y(), threadIndex, log);
			{
				std::unique_lock<std::mutex> lock(this->mx_completion);

				//this->threadCompletionRegister[rowIndex]++;
				this->threadCompletedCounter++;
				this->cv_completion.notify_one();

				log << Log::Color::Red << "Section " << TaskInfo.x() << " completed. threadCompleteCounter: " << this->threadCompletedCounter.load() << Log::print;
				
				//this->threadUpdateTime += timer.durationL();
			}
		}
	}
	
	void threadProcessSection(int sectionIndex, int len, int threadIndex, Log& log) {
		log << "thread " << threadIndex << " started at section " << sectionIndex << " with " << len << Log::print;
		int index = sectionIndex * this->threadSectionHeight * this->gridRowAmount;
		len -= (this->gridRowAmount + this->gridRowAmount);

		for (int i = 0; i < this->gridRowAmount; i++) {
			processGridCollision(index, threadIndex);
			index++;
		}
		if (sectionIndex) { // declare first row finished
			std::unique_lock<std::mutex> lock(this->mx_borderClear);
			log << Log::Color::Green << "thread " << threadIndex << " declare border clear." << Log::print;
			this->threadBorderClearDeclaration[sectionIndex - 1] = 1;
			this->cv_borderClear.notify_all();
			
		}
		for (int i = 0; i < len; i++) {
			processGridCollision(index, threadIndex);
			index++;
		}
		{ // wait for the section below it to finish first row
			std::unique_lock<std::mutex> lock(this->mx_borderClear);

			log << Log::Color::Blue << "thread " << threadIndex << " waiting..." << sectionIndex << this->threadBorderClearDeclaration[sectionIndex] << Log::print;
			this->cv_borderClear.wait(lock, [this, sectionIndex]() {
				return this->threadBorderClearDeclaration[sectionIndex];
				});
		}
		log << Log::Color::Yellow << "thread " << threadIndex << " woke up with index: " << index << ", index in section: " << (index - sectionIndex * this->threadSectionHeight * this->gridRowAmount) << Log::print;
		for (int i = 0; i < this->gridRowAmount; i++) {
			processGridCollision(index, threadIndex);
			index++;
		}

		log << Log::Color::Magenta << "thread " << threadIndex << " end." << Log::print;
		

		/*for (int i = 0; i < this->gridRowAmount; i++) {
			const std::array<int, GridMaxOccupation>& gridEntities = this->Grid[start + i];
			if (gridEntities.size() == 0) continue;
			this->maxGridObjectCount = max(maxGridObjectCount.load(), (int)gridEntities.size());
			const Coord gridPos(rowIndex, i);

			vector<const vector<int>*> surroundingGridEntities;
			for (const Coord& inc : this->gridIncrement) {
				Coord surroundingPos = gridPos + inc;
				if (surroundingPos.valid(this->gridRowAmount)) {
					surroundingGridEntities.push_back(&this->Grid[surroundingPos.getX()][surroundingPos.getY()]);
				}
			}

			processGridCollision(gridEntities, surroundingGridEntities);
		}*/
	} // update: add side grids to get rid of valid() decision ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	void threadSubmitUpdateTask(int startIndex, int len) {
		std::lock_guard<std::mutex> lock(this->mx_worker);
		this->threadProcessQueue.push({ startIndex, len });
		this->cv_sendTask.notify_one();
	}

	void updateCollision() {
		//Timer timer("UpdateCollision", NoEndLog);
		Log log;
		for (int i = 0; i < this->fullSectionAmount; i++) {
			threadSubmitUpdateTask(i, this->fullSectionLength);
			log << "task submitted." << Log::print;
		}
		if (this->remainingLength) {
			threadSubmitUpdateTask(this->fullSectionAmount, this->remainingLength);
			log << "task submitted." << Log::print;
		}
		std::unique_lock<std::mutex> lock(this->mx_completion);
		this->cv_completion.wait(lock, [this]() {
			return this->threadCompletedCounter == this->totalSectionAmount;
			});

		this->threadCompletedCounter = 0;
		this->threadUpdateTime = 0;
		std::fill(this->threadBorderClearDeclaration.begin(), this->threadBorderClearDeclaration.end() - 1, 0);
	}

	// Grid System ************************************************************************************************************
	
	/*const vector<Coord> gridIncrement = {
		{ 1,  0},
		{ 1,  1},
		{ 0,  1},
		{-1,  1},
		{-1,  0},
		{-1, -1},
		{ 0, -1},
		{ 1, -1}
	};*/
	const vector<Coord> gridAdjacentIncrement = {
		{ 1,  1},
		{ 0,  1},
		{-1,  1},
		{-1,  0}
	};
	/*const vector<Coord> gridAdjacentIncrement = {
		{-1,  1},
		{-1,  0},
		{-1, -1},
		{ 0, -1}
	};*/

	inline int gridGetLinearIndex(const Coord& in) { return in.x() * this->gridRowAmount + in.y() ; }

	void gridInitSide(Coord start, Coord increment) {
		for (int i = 0; i < this->gridRowAmount; i++) {
			int linearIndex = gridGetLinearIndex(start);
			this->gridSide.push_back(&this->Grid[linearIndex]);
			
			for (int j = 0; j < this->gridAdjacentIncrement.size(); j++) {
				Coord adjacentGrid = start + gridAdjacentIncrement[j];
				if (!adjacentGrid.valid(this->gridRowAmount)) continue;
				this->gridAdjacent[linearIndex][j] = &this->Grid[gridGetLinearIndex(adjacentGrid)];
			}			
			start += increment;
		}
	}
	void gridInit(float fixedRadius) {
		this->gridRowAmount = std::ceilf(1.0f / fixedRadius);
		this->gridSideLen = fixedRadius * 2;
		this->Grid.assign(sq(this->gridRowAmount), std::array<int, GridMaxOccupation>{});
		this->fullSectionAmount = this->gridRowAmount / this->threadSectionHeight;
		this->fullSectionLength = this->threadSectionHeight * this->gridRowAmount;
		this->remainingLength = (this->gridRowAmount - fullSectionAmount * this->threadSectionHeight) * this->gridRowAmount;
		this->totalSectionAmount = fullSectionAmount + (this->remainingLength ? 1 : 0);
		// Grid Adjacent init
		this->gridAdjacent = vector<std::array<std::array<int, GridMaxOccupation>*, 4>>(sq(this->gridRowAmount));
		int gridInnerRowAmount = this->gridRowAmount - 1;
		for (int x = 1; x < gridInnerRowAmount; x++) {
			int start = x * this->gridRowAmount;
			for (int y = 1; y < gridInnerRowAmount; y++) {
				for (int i = 0; i < this->gridAdjacentIncrement.size(); i++) {
					this->gridAdjacent[start + y][i] = &this->Grid[gridGetLinearIndex(Coord(x + this->gridAdjacentIncrement[i].x(), y + this->gridAdjacentIncrement[i].y()))];
				}
			}
		}
		// side grid init
		this->gridSide.reserve(this->gridRowAmount * 4);
		gridInitSide({ 0, 0 }, { 1, 0 });
		gridInitSide({ 0, 0 }, { 0, 1 });
		gridInitSide({ gridInnerRowAmount, gridInnerRowAmount }, { -1,  0 });
		gridInitSide({ gridInnerRowAmount, gridInnerRowAmount }, {  0, -1 });

		




	}
	//inline int gridToGridPos(float in) { return ((in + 1.0f) / 2.0f * this->gridRowAmount); }
	inline int gridToGridPos(float in) { return max(min((int)((in + 1.0f) / 2.0f * this->gridRowAmount), this->gridRowAmount - 1), 0); }
	inline Coord gridGetCurrentPos(const PhysicalObject& obj) {
		return Coord(gridToGridPos(obj.getX()), gridToGridPos(obj.getY()));
	}
	inline void gridAddObj(Coord gridPos, PhysicalObject& obj, int objIndex) {
		obj.gridPos = gridPos;
		std::array<int, GridMaxOccupation>& arr = this->Grid[gridGetLinearIndex(gridPos)];
		arr[arr[GridSizeIndex]] = objIndex; // potential error of overflow ----------------------------------------------------
		obj.gridIndex = arr[GridSizeIndex];
		arr[GridSizeIndex]++;

		

		//cout << arr[GridSizeIndex] << '\n';
		if (arr[GridSizeIndex] == GridSizeIndex) {
			int i = 0;
			cout << Log::Color::Bold << Log::Color::Red << "------- FATAL ERROR: Grid Index Exceeded -------\n";
		}
	}
	inline void gridMoveObj(Coord destination, Coord currentPos, PhysicalObject& obj, int objIndex) {
		int previousGridIndex = obj.gridIndex;
		gridAddObj(destination, obj, objIndex);
		// delete the original
		std::array<int, GridMaxOccupation>& arr = this->Grid[gridGetLinearIndex(currentPos)]; int arr_size = arr[GridSizeIndex];
		if (previousGridIndex != arr_size - 1) {
			this->entities[arr[arr_size - 1]].gridIndex = previousGridIndex;
			std::swap(arr[previousGridIndex], arr[arr_size - 1]);
		} arr[GridSizeIndex]--;

	}

	void gridUpdate() {
		for (int i = 0; i < this->entities.size(); i++) {
			gridUpdateObj(i);
		}
	}
	void gridUpdateObj(int index) {
		PhysicalObject& obj = this->entities[index];
		Coord currentPos = gridGetCurrentPos(obj);
		if (obj.gridPos != currentPos) {
			if (obj.gridPos.getX() == -1) { // not yet putted into the grid sys (both index set to -1, which is not allowed in the grid sys)
				gridAddObj(currentPos, obj, index);
			}
			else {
				gridMoveObj(currentPos, obj.gridPos, obj, index);
			}
		}
	}

	
	// Physics ****************************************************************************************************************
	// Collision **************************************************************************************************************
	bool isColliding(const PhysicalObject& a, const PhysicalObject& b) {
		return (sq(a.getX() - b.getX()) + sq(a.getY() - b.getY())) < sq(a.getR() + b.getR() - this->Slop);
	}
	
	
	void processGridCollision(int gridIndex, int threadIndex) {
		//cout << gridIndex << endl;
		std::array<int, GridMaxOccupation>& grid = this->Grid[gridIndex]; int grid_size = grid[GridSizeIndex];
		std::array<std::array<int, GridMaxOccupation>*, 4>& adjacentGrid = this->gridAdjacent[gridIndex];
		
		for (int i = 0; i < grid_size; i++) {
			PhysicalObject& obja = this->entities[grid[i]];
			for (int j = i + 1; j < grid_size; j++) {
				PhysicalObject& objb = this->entities[grid[j]];
				if (!isColliding(obja, objb)) continue;
				solveCollision(obja, objb);
				//gridUpdateObj(grid[i]);
				//gridUpdateObj(grid[j]);
			}
			for (int directionIndex = 0; directionIndex < 4; directionIndex++) {
				std::array<int, GridMaxOccupation>& agrid = *adjacentGrid[directionIndex];
				if (&agrid == nullptr) continue;
				int agrid_size = agrid[GridSizeIndex];
				for (int j = 0; j < agrid_size; j++) {
					PhysicalObject& objb = this->entities[agrid[j]];
					if (!isColliding(obja, objb)) continue;
					solveCollision(obja, objb);
					//gridUpdateObj(grid[i]);
					//gridUpdateObj(agrid[j]);
				}
			}
		}
		/*for (int i = 0; i < in_entities.size(); i++) {
			PhysicalObject& obja = this->entities[in_entities[i]];
			for (int j = i + 1; j < in_entities.size(); j++) {
				PhysicalObject& objb = this->entities[in_entities[j]];
				if (!isColliding(obja, objb)) continue;
				else solveCollision(obja, objb);
			}
			for (const vector<int>* j : in_contactEntities) {
				const vector<int>& vec = *j;
				for (int index : vec) {
					PhysicalObject& objb = this->entities[index];
					if (!isColliding(obja, objb)) continue;
					else solveCollision(obja, objb);
				}
			}
		}*/
	}
	// 你以为这是 Vertet 被我调教的差不多了？不不不，这是我被 Verlet 调教的差不多了。
	void solveCollision(PhysicalObject& obja, PhysicalObject& objb) {
		// basic variables
		vec2 deltaVec = obja.getPos() - objb.getPos();
		float distance = deltaVec.length();
		float radiusSum = obja.getR() + objb.getR();
		float overlap = radiusSum - distance - this->Slop;

		if (overlap <= 0.0f) // if not overlapping
			return;

		// logic variable
		vec2 normalUnitVec = unify(deltaVec);

		// solve overlap
		vec2 overlapCorrection = overlap * 0.5f * normalUnitVec * OverlapAdjustPercentage;
		obja.position += overlapCorrection;
		objb.position -= overlapCorrection;

		// logic variable for solving bounce
		vec2 relativeVelocity = obja.getVelocity() - objb.getVelocity();
		float normalVelocity = dot(relativeVelocity, normalUnitVec);

		if (normalVelocity >= 0.0f) // if normal relative velocity is not toward each other
			return;

		// solve bounce
		float bounceImpulse = normalVelocity * this->Restitution;
		vec2 impulseVec = normalUnitVec * bounceImpulse;
		obja.lastPosition += impulseVec;
		objb.lastPosition -= impulseVec;

		//// --- Basic Variables & Mass ---
		//vec2 deltaVec = obja.getPos() - objb.getPos();
		//float distance = deltaVec.length();
		//float radiusSum = obja.getR() + objb.getR();
		//float overlap = radiusSum - distance - this->slop;

		//// Only proceed if overlap is positive (meaning they are penetrating)
		//if (overlap < 0.0f)
		//	return;

		//vec2 normalUnitVec = unify(deltaVec); // Normal points from B to A

		//// Using radius as a mass proxy (mass proportional to R^2).
		//float massA = obja.getR() * obja.getR();
		//float massB = objb.getR() * objb.getR();
		//float massSum = massA + massB;

		//// Inverse mass ratios (crucial for stable constraint distribution)
		//float inverseMassA = massB / massSum;
		//float inverseMassB = massA / massSum;

		//// --- 1. Positional Correction (Solving Overlap - STABILITY FIX) ---
		//// Distribute correction based on inverse mass ratios. This is required 
		//// for stability against high external forces/constraints.
		//vec2 correctionA = normalUnitVec * (overlap * inverseMassA);
		//vec2 correctionB = normalUnitVec * (overlap * inverseMassB);

		//obja.position += correctionA; // Move A away from B
		//objb.position -= correctionB; // Move B away from A

		//// --- 2. Impulse Injection (Solving Bounce) ---

		//// Calculate the speed of approach along the normal
		//vec2 relativeVelocity = obja.getVelocity() - objb.getVelocity();
		//float normalVelocity = dot(relativeVelocity, normalUnitVec);

		//// Only apply bounce impulse if they are closing (negative V_normal)
		//if (normalVelocity >= 0.0f)
		//	return;

		//// Calculate Total required impulse magnitude (I_total)
		//float impulseMagnitude = -(1.0f + this->restitution) * normalVelocity;

		//// Distribute the impulse based on inverse mass ratios
		//float impulseA_mag = impulseMagnitude * inverseMassA;
		//float impulseB_mag = impulseMagnitude * inverseMassB;

		//vec2 impulseVecA = normalUnitVec * impulseA_mag;
		//vec2 impulseVecB = normalUnitVec * impulseB_mag;

		//// Apply the impulse to lastPosition to force the bounce velocity
		//obja.lastPosition -= impulseVecA;
		//objb.lastPosition += impulseVecB;
	}
	void solveCollision(vector<PhysicalObject>& in_entities) {
		if (in_entities.size() < 2) return;
		for (int i = 0; i < in_entities.size(); i++) {
			PhysicalObject& obja = in_entities[i];
			for (int j = i + 1; j < in_entities.size(); j++) {
				PhysicalObject& objb = in_entities[j];

				if (!isColliding(obja, objb)) continue;

				// basic variables
				vec2 deltaVec = obja.getPos() - objb.getPos();
				float distance = deltaVec.length();
				float radiusSum = obja.getR() + objb.getR();
				float overlap = radiusSum - distance - this->Slop;

				if (overlap <= 0.0f) // if not overlapping
					continue;

				// logic variable
				vec2 normalUnitVec = unify(deltaVec);

				// solve overlap
				vec2 overlapCorrection = overlap * 0.5f * normalUnitVec * OverlapAdjustPercentage;
				obja.position += overlapCorrection;
				objb.position -= overlapCorrection;

				// logic variable for solving bounce
				vec2 relativeVelocity = obja.getVelocity() - objb.getVelocity();
				float normalVelocity = dot(relativeVelocity, normalUnitVec);

				if (normalVelocity >= 0.0f) // if normal relative velocity is not toward each other
					continue;

				// solve bounce
				//float bounceImpulse = normalVelocity * this->restitution;
				float bounceImpulse = (1.0f + this->Restitution) * normalVelocity * 0.5f;
				vec2 impulseVec = normalUnitVec * bounceImpulse;
				obja.lastPosition += impulseVec;
				objb.lastPosition -= impulseVec;
			}
		}
	}
	const vector<vec2> borderReflectCoef = {
	{  0.0f,  1.0f }, // bottom
	{  1.0f,  0.0f }, // left
	{  0.0f, -1.0f }, // top
	{ -1.0f,  0.0f }  // right
	};
	void solveBorderCollision() {
		int index = 0;
		for (int i = 0; i < 4; i++) {
			vec2 normalUnitVec = borderReflectCoef[i];
			for (int j = 0; j < this->gridRowAmount; j++) {
				std::array<int, GridMaxOccupation>& gridArr = *this->gridSide[index]; int arr_size = gridArr[GridSizeIndex];
				for (int k = 0; k < arr_size; k++) {
					PhysicalObject& obj = this->entities[gridArr[k]];
					// solve overlap
					float border = 1.0f - obj.radius;
					float overlap = std::fabs(dot(normalUnitVec, obj.position)) - border;
					float slop = 0.0001f;
					if (overlap <= slop) continue;

					obj.position += normalUnitVec * overlap * this->OverlapAdjustPercentage;

					// adjust velocity
					float diff = dot(normalUnitVec, obj.position - obj.lastPosition);
					if (diff >= 0) continue;
					obj.lastPosition += normalUnitVec * diff * 2 * this->Restitution;
				} index++;
			}			
		}
	}













	// Gravity *******************************************************************************************************************
	void applyGravity() {
		for (PhysicalObject& i : this->entities) {
			for (PointObject* j : this->gravityPoints) {
				float dist;
				vec2 directionVec = unify(j->getPos() - i.getPos(), &dist);
				if(dist < this->Gravity)
					i.accelerate(directionVec * dist * 0.5f);
				else
					i.accelerate(directionVec * this->Gravity);
			}
		}
	}
	



};





// updates:
// 1. Change grid update into reset
// 2. Change all pointer & refernces to index
// 3. Costom border & shape (static body)
// 4. string
// 
// 
// 
//

// bugs:
// 1. center jitter: fixed by changing the value of overlap adjustion from 100% to 50%
// 2. 
// 
// 
// 
// 
// 
// 
//







int main() {

	


	GLFWwindow* window;
	/* Initialize the library */
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_SAMPLES, 4);
	// Create a windowed mode window and its OpenGL context
	//window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	// 
	//GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor(); // main monitor
	//const GLFWvidmode* primaryMonitorMode = glfwGetVideoMode(primaryMonitor);
	//window = glfwCreateWindow(primaryMonitorMode->width, primaryMonitorMode->height, "Project by TX_Jerry", primaryMonitor, NULL);

	window = glfwCreateWindow(900, 900, "Project by TX_Jerry", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	windowG = window;

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	

	// adjust window position
	glfwSetWindowPos(window, 5, 42);
	//glfwSetWindowPos(window, 0, 0);

	// bgm
#ifdef USING_BGM
	ma_engine mengine;
	ma_engine_init(NULL, &mengine);
	ma_decoder decoder;
	ma_decoder_init_memory(rawData, dataLen, NULL, &decoder);
	ma_sound sound;
	ma_sound_init_from_data_source(&mengine, &decoder, MA_SOUND_FLAG_LOOPING, NULL, &sound);

	ma_sound_start(&sound);
#endif
	//cout << "Press Enter to start.\n";
	//cin.get();

	// Basic variables ********************************************************************************************************

	int width, height;
	glfwGetWindowSize(window, &width, &height);
	//cout << "width: " << width << " height: " << height << endl;

	unsigned long long tickCounter = 0;

	debugLogInit();

	// Logic Variables ********************************************************************************************************

	GravitySys engine(100000, 6, 6, 0.0001f, 0.9f, 0.0f, 0.999f, 1.0f);
	engine.enableGridSystem(0.006f);
	engine.enableMultiThread(20);
	//engine.enableFriciton();
	
	//const float entityDiameter = 0.01f;
	const float entityDiameter = 0.02f;
	int entityAmount = std::pow(2.0f / entityDiameter, 2) - 10, intervalTick = 10;
	int entityAmountCounter = 0;
	
	entityAmount = 32950;

	float standardSpeed = 0.01f;
	float modifier = 3.0f;


	float highestY = 0.95f;
	double mouseX, mouseY;



	vector<PhysicalObject*> entities;
	entities.reserve(entityAmount);
	vector<RGB> entityColor;
	entities.reserve(entityAmount);

	std::random_device rrde;
	std::mt19937 rde(rrde());
	std::uniform_real_distribution<float> dist_spawnRange(-1.0, 1.0);
	std::uniform_real_distribution<float> dist_smallSpawnRange(-0.07, 0.07);
	std::uniform_real_distribution<float> dist_spawnVelocity(-0.001, 0.001);

	int amountF = 20;
	vector<vec2> coords;
	for (int i = 0; i < amountF; i++) {
		coords.push_back(vec2(-0.99f, 0.99f - i * 0.02f));
	}

	int amount = 20;
	

	Rainbow rgbEngine(36000);

	// pyramid test
	//entities.push_back(engine.createPhysicalObject( 0.15f, -0.85f, 0.15f, 0.0f, 0.0f, EntityType::Falling));
	//entities.push_back(engine.createPhysicalObject(-0.15f, -0.85f, 0.15f, 0.0f, 0.0f, EntityType::Falling));
	//entities.push_back(engine.createPhysicalObject(0.0f, 0.0f, 0.15f, 0.0f, 0.02f, EntityType::Falling));

	//entities.push_back(engine.createPhysicalObject(0.0f, -0.85f, 0.1f, 0.0f, 0.01f, EntityType::Floating));
	//entities.push_back(engine.createPhysicalObject( 0.3f, 0.0f, 0.1f, -0.02f, 0.0f, EntityType::Floating));
	//entities.push_back(engine.createPhysicalObject(-0.3f, 0.0f, 0.1f, 0.02f, 0.0f, EntityType::Floating));



	//PointObject* gravityPoint1 = engine.createGravityPoint( 0.8f,  0.8f);
	//PointObject* gravityPoint2 = engine.createGravityPoint(-0.8f, -0.8f);
	//PointObject* gravityPoint = engine.createGravityPoint(0.0f, 0.0f);

	/*for (int i = 0; i < entityAmount; i++) {
		entities.push_back(engine.createPhysicalObject(dist_spawnRange(rde), dist_spawnRange(rde), entityDiameter / 2, 0.0f, 0.0f, EntityType::Falling));

	}
	
	engine._gridUpdate();*/

	cout << "Welcome to TXPE, the Physics Engine made by TX Studio!\n"
		<< "Please enter the object spwan rate (1 ~ 1000): ";
	cin >> amount;



	cout << "Main Loop Start\n";
	// Main Loop
	timeBeginPeriod(1);
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
#ifdef FIXED_TICKRATE
	std::chrono::steady_clock::time_point last = start;
	double accumulator = 0.0;
#endif
	while (!glfwWindowShouldClose(window)) {
		bool isTerminated = 0;
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
#ifndef FIXED_TICKRATE
		cin.get();
#endif
#ifdef FIXED_TICKRATE
		double tick_duration = std::chrono::duration<double>(now - last).count();
		accumulator += tick_duration;
		last = now;
		if (accumulator > maxAccumulatorTime) accumulator = maxAccumulatorTime;	
		while (accumulator >= tickIntervalTime)
#endif
		{
			// calculation
			engine.update();


			/*glfwGetCursorPos(window, &mouseX, &mouseY);
			mouseX = (mouseX - width / 2) / (width / 2);
			mouseY = -(mouseY - height / 2) / (height / 2);
			gravityPoint->setX(mouseX);
			gravityPoint->setY(mouseY);*/

			
			//if (!(tickCounter % intervalTick) && entityAmountCounter < entityAmount) {
			//	//entities.push_back(engine.createPhysicalObject(-0.8f, 0.8f, entityDiameter / 2, 0.02f, 0.0f, EntityType::Falling));
			//	//entities.push_back(engine.createPhysicalObject(0.0f, 0.8f, entityDiameter / 2, 0.0f, 0.0f, EntityType::Falling));
			//	entities.push_back(engine.createPhysicalObject(dist_spawnRange(rde), dist_spawnRange(rde), entityDiameter / 2, 0.0f, 0.0f, EntityType::Floating));



			//	entityAmountCounter++;
			//}

			if (entityAmountCounter < entityAmount) {
				/*for (vec2& i : coords) {
					entities.push_back(engine.createPhysicalObject(i.getX(), i.getY(), 0.0f, 0.012f, 0.05f, EntityType::Floating));
					entityColor.push_back(rgbEngine.getNextColor());
					entities.push_back(engine.createPhysicalObject(i.getX() * -1.0f, i.getY(), 0.0f, -0.012f, 0.05f, EntityType::Floating));
					entityColor.push_back(rgbEngine.getNextColor());


					entities.push_back(engine.createPhysicalObject(i.getX(), i.getY() * -1.0f, 0.0f, 0.012f, -0.05f, EntityType::Floating));
					entityColor.push_back(rgbEngine.getNextColor());
					entities.push_back(engine.createPhysicalObject(i.getX() * -1.0f, i.getY() * -1.0f, 0.0f, -0.012f, -0.05f, EntityType::Floating));
					entityColor.push_back(rgbEngine.getNextColor());
				}*/

				
				for (int i = 0; i < amount; i++) {
					entities.push_back(engine.createPhysicalObject(dist_spawnRange(rde), 0.98f, 0.0f, 0.0f, 0.0f, EntityType::Falling));
					entityColor.push_back(rgbEngine.getNextColor());
				}


				entityAmountCounter += amount;

				

			}






			tickCounter++;
#ifdef FIXED_TICKRATE
			accumulator -= tickIntervalTime;
#endif
		}
		if (isTerminated)
			break;



		glClear(GL_COLOR_BUFFER_BIT);
		// render start
		glBegin(GL_TRIANGLES);
		/*
		for (const coord& i : coords) {
			drawSquare(obj.getR() * 2, i.getX(), i.getY());
		}*/
		//drawSquare(strikeObj.getR() * 2, strikeObj.getX(), strikeObj.getY());
		





		/*drawCircle(staticObj.getX(), staticObj.getY(), staticObj.getR());*/
		//drawSquare(staticObj.getR() * 2, staticObj.getX(), staticObj.getY());
		{
			//Timer timer("Render");
			for (int i = 0; i < entities.size(); i++) {
#ifdef FIXED_TICKRATE
				//double alpha = accumulator / tickIntervalTime;
				//float x = i->getlpx() * (1.0 - alpha) + i->getX() * alpha;
				//float y = i->getlpy() * (1.0 - alpha) + i->getY() * alpha;

				//drawCircle(x, y, i->getR() * 1.3f); // * 1.3
				const RGB& color = entityColor[i];
				glColor3f(color.r(), color.g(), color.b());
				drawCircle(entities[i]->getX(), entities[i]->getY(), entities[i]->getR());
#endif
#ifndef FIXED_TICKRATE
				drawCircle(i->getX(), i->getY(), i->getR());
#endif
			}
		}
		/*for (int i = 0; i < entities.size(); i++) {
			if (entities[i].getX() == 0.0f && entities[i].getY() == 0.0f) {
				continue;
			}
			drawCircle(entities[i].getX(), entities[i].getY(), entities[i].getR());
		}*/
		

		





		// render end
		glEnd();

		// line system ********************************************************************************************************
		/*glBegin(GL_LINES);

		glVertex2f(-1.0f, highestY);
		glVertex2f(1.0f, highestY);

		glEnd();*/
		// line system ends ***************************************************************************************************

		/* Swap front and back buffers */
		glfwSwapBuffers(window);
		/* Poll for and process events */
		glfwPollEvents();
	}
#ifdef USING_BGM
	ma_sound_stop(&sound);
	ma_sound_uninit(&sound);
	ma_decoder_uninit(&decoder);
	ma_engine_uninit(&mengine);
#endif
	timeEndPeriod(1);
	glfwTerminate();
	return 0;
}