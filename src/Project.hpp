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
	cout << "init json...\n";
	string str;
	tx::readWholeFile(filePath, str);

	tx::JsonObject root = tx::JsonObject{};
	tx::JsonParser parser;
	cout << "start init json.\n";
	parser.parse(str, root);
	cout << "init json done.\n";
}


class BMPFile {
public:
	// headers **********************************************************
    // ================= BMP FILE HEADER =================
// Fixed size: 14 bytes
// Purpose: describes the FILE, not the image itself

// Signature of the file.
// Must be 'BM' (0x4D42, little-endian).
// This is how you know the file is a BMP at all.
uint16_t bfType;

// Total size of the BMP file in bytes.
// Includes headers, palettes, padding, and pixel data.
// Rarely needed in loaders, mostly informational.
uint32_t bfSize;        

// Reserved by Microsoft.
// Must be zero.
// You ignore these, but non-zero usually means malformed file.
uint16_t bfReserved1;  
uint16_t bfReserved2;  

// Offset (in bytes from beginning of file) to the FIRST pixel.
// This is the most important field in the file header.
// You MUST seek to this location before reading pixel data.
// Everything between the start of the file and bfOffBits is "metadata".
uint32_t bfOffBits;    


// ================= DIB HEADER =================
// Variable size (at least 12 bytes, usually 40)
// Purpose: describes the IMAGE layout

// Size of the DIB header in bytes.
// Determines which DIB format is used:
//  40  = BITMAPINFOHEADER (classic, what you want)
// 108  = BITMAPV4HEADER
// 124  = BITMAPV5HEADER
// Never assume 40 — always read this.
uint32_t dibSize;       

// Raw bytes of the DIB header AFTER dibSize.
// Size = dibSize - 4.
// Stored raw so:
//  - you support future BMP variants
//  - you don’t need different structs per version
// You selectively parse fields you care about.
std::vector<uint8_t> dibData; 


// ========== COMMONLY USED IMAGE FIELDS ==========
// These exist starting from BITMAPINFOHEADER (dibSize >= 40)
// Offsets are RELATIVE TO dibData (not file start)

// Image width in pixels.
// Positive value only.
// This defines how many pixels per row.
int32_t width = 0;      

// Image height in pixels.
// IMPORTANT SIGN MEANING:
//   > 0 : image is bottom-up (row 0 = bottom row)
//   < 0 : image is top-down  (row 0 = top row)
// Magnitude is the pixel height.
int32_t height = 0;     

// Must be 1.
// Historical artifact from old hardware.
// If not 1, the file is invalid.
uint16_t planes = 0;   

// Bits per pixel.
// Common values:
//   24 = BGR (no alpha)
//   32 = BGRA
//   8  = paletted
// This tells you how many BYTES each pixel occupies.
uint16_t bitCount = 0; 

// Compression method.
// Common values:
//   0 = BI_RGB (no compression) ← what you want
//   1 = BI_RLE8
//   2 = BI_RLE4
//   3 = BI_BITFIELDS
// Anything non-zero usually complicates loading.
uint32_t compression = 0;


	bool valid = 1;

	BMPFile(const std::filesystem::path& fp) {
		this->ifs = std::ifstream{fp};
		if (!ifs) {
			valid = 0;
			return;
		}

		// --- FILE HEADER
		ifs.read(reinterpret_cast<char*>(&bfType), 2);
		ifs.read(reinterpret_cast<char*>(&bfSize), 4);
		ifs.read(reinterpret_cast<char*>(&bfReserved1), 2);
		ifs.read(reinterpret_cast<char*>(&bfReserved2), 2);
		ifs.read(reinterpret_cast<char*>(&bfOffBits), 4);

		if (bfType != 0x4D42) {
			valid = 0;
			return;
		} // "BM"

		// --- DIB HEADER SIZE
		ifs.read(reinterpret_cast<char*>(&dibSize), 4);
		if (dibSize < 4) {
			valid = 0;
			return;
		}

		// --- DIB HEADER RAW DATA
		dibData.resize(dibSize - 4);
		ifs.read(reinterpret_cast<char*>(dibData.data()), dibData.size());

		// --- Parse common fields if BITMAPINFOHEADER or larger
		if (dibSize >= 40) {
			std::memcpy(&width,       dibData.data() + 0,  4);
			std::memcpy(&height,      dibData.data() + 4,  4);
			std::memcpy(&planes,      dibData.data() + 8,  2);
			std::memcpy(&bitCount,    dibData.data() + 10, 2);
			std::memcpy(&compression, dibData.data() + 12, 4);
		}

		// --- Skip anything between headers and pixels
		ifs.seekg(bfOffBits, std::ios::beg);
	}


	std::ifstream& getIfs(){
		return ifs;
	}
private:
	std::ifstream ifs;
	



};

using bmp = tx::GridSystem<tx::RGB>;
bmp readBMP(const std::fs::path& fp) {
	BMPFile file{fp};
	
	bmp map{file.width, file.height};

    std::ifstream& ifs = file.getIfs();

    const int width  = file.width;
    const int height = std::abs(file.height);
    const bool topDown = file.height < 0;

    const int rowStride = ((width * 3 + 3) / 4) * 4;
    std::vector<uint8_t> row(rowStride);

    for (int bmpY = 0; bmpY < height; ++bmpY) {
        int y = topDown ? (height - 1 - bmpY) : bmpY;

        ifs.read(reinterpret_cast<char*>(row.data()), rowStride);

        for (int x = 0; x < width; ++x) {
            uint8_t b = row[x * 3 + 0];
            uint8_t g = row[x * 3 + 1];
            uint8_t r = row[x * 3 + 2];

            map.set(x, y, tx::RGB(
                r, g, b
            ));
        }
    }

	return map;
}



enum class TileType{
	Space,
	Ore_Iron,
	Ore_Copper,
	Ore_Coal,
	Ore_Gold

};



class Tile {
public:



	void set(TileType in) { type = in; }
private:
	TileType type;


};


// tile amount: 64
class Game {
public:
	Game() : 
		rde([](){
			std::random_device rrde;
			std::mt19937 rde_{rrde()};
			return rde_;
		}())
	{
		tiles.reinit(MapSize);
		genOreTiles_impl();
	}

	void update(){

	}

	void render(){
		for(int y = 0; y < MapSize; ++y){
			for(int x = 0; x < MapSize; ++x){
				
			}
		}


	}



private:
	// runtime data
	tx::GridSystem<Tile> tiles;
	//vector<int> extractors;
	vector<int> ores;

private:
	// config
	tx::JsonObject cfg = initJsonObject("./config.json");
	int MapSize = 64;
	float TileSize = 2.0f / MapSize;
private:
	// utility
	std::mt19937 rde;
	std::uniform_int_distribution<int> dist_map{0, MapSize - 1};


	void setOreTile_impl(const tx::Coord& pos, TileType type) {
		if(!valid_impl(pos)) return;
		tiles.at(pos).set(type);
		ores.push_back(tiles.index(pos));
	}

	





	void genOreTiles_impl() {
		genOre_PolicyCommon_impl(TileType::Ore_Coal);
	}
	void genOre_PolicyCommon_impl(TileType type) {
		static tx::JsonObject policyCfg = cfg["OreGeneration"]["PolicyCommon"].get<tx::JsonObject>();
		static tx::Bitmap circle;
		static int radius = [&](){
			cout << "radius\n";
			int r = policyCfg["radius"].get<int>(); // 5
			tx::GridCircle gc{r};
			gc.getBitMap(tx::CoordOrigin, circle);
			return r;
		}();
		static tx::Bitmap surroundingCircle;
		static int surroundingRadius = [&](){
			cout << "sradius\n";
			int sr = policyCfg["surroundingClusterRadius"].get<int>();
			tx::GridCircle gc{sr};
			gc.getBitMap(tx::CoordOrigin, surroundingCircle);
			return sr;
		}();
		static std::uniform_int_distribution<int> dist_offset{0, 5/*policyCfg["surroundingClusterOffset"].get<int>*/};

		tx::Coord center = getRandCoord_impl();
		vector<tx::Coord> surroundingCenters{policyCfg["surroundingClusterAmount"].get<int>()}; // 3
		for(tx::Coord& i : surroundingCenters){
			i = center.offset(dist_offset(rde), dist_offset(rde));
		}

		//tx::Bitmap map{(1 + surroundingCenters.size()) * circle.size()};
		
		auto setOreTiles = [&](const tx::Coord& pos, const tx::Bitmap& map){
			for(const tx::Coord& i : map){
				setOreTile_impl(i + pos, type);
			}
		};
		
		setOreTiles(center, circle);
		for(const tx::Coord& i : surroundingCenters){
			setOreTiles(i, surroundingCircle);
		}
	}

	tx::Coord getRandCoord_impl() {
		return tx::Coord{dist_map(rde), dist_map(rde)};
	}
	bool valid_impl(const tx::Coord& in) const {
		tx::inRange(in, tx::CoordOrigin, tx::Coord{MapSize});
	}



	

	// render ********************************

	void renderTile_impl(const tx::Coord& in_pos, TileType type){
		tx::vec2 pos = tx::toVec2(in_pos) * TileSize;

		// switch(type){
		// case TileType::Ore_Coal:

		// 	break;
		// case TileType::Ore_Copper:

		// 	break;
		// case TileType::Ore_Gold:

		// 	break;
		// case TileType::Ore_Iron:

		// 	break;
		// case TileType::Space:

		// 	break;
		// }

		tx::drawRectP(pos, TileSize, TileSize);
		
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