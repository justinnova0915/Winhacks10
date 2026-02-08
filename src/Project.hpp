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
void initJsonObject(const std::filesystem::path& filePath, tx::JsonObject& root) {
    cout << "init json...\n";
    string str;
    tx::readWholeFile(filePath, str);

    root = tx::JsonObject{};
    tx::JsonParser parser;
    cout << "start init json.\n";
    parser.parse(str, root);
    cout << "init json done.\n";
}

enum class CoordDirection : uint16_t {
    Right       = 0,        
    Left        = 1,         
    Top         = 2,          
    Bottom      = 3,       
    TopRight    = 4,     
    TopLeft     = 5,      
    BottomLeft  = 6,   
    BottomRight = 7,
    None        = 255  // No conveyor on this tile
};
constexpr tx::Coord dirToCoord(CoordDirection dir) {
    return tx::_8wayIncrement[static_cast<int>(dir)];
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

using RGBMap = tx::GridSystem<tx::RGB>;
RGBMap readBMP(const std::fs::path& fp) {
    BMPFile file{fp};
    
    RGBMap map{file.width, file.height};

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
    //map.foreach([](const tx::RGB& in){ cout << in; });
    return map;
}

struct Entity {
    float distance = 0.0f;
    float size = 1.0f;
    uint16_t id = 0;
};

class ConveyorSegment{
    public:

        void update(float dt, float speed) {
            if (entities.empty()) return;

            Entity& head = entities.front();
            head.distance += speed * dt;

            if (head.distance >= length) {
                if (nextsegment && !nextsegment->isEntryBlocked()) {
                    Entity transfer = head;
                    transfer.distance = 0.0f;
                    nextsegment->entities.push_back(transfer);  // Add to front (head) of next segment

                    entities.pop_front();

                    if (entities.empty()) return;
                } else {
                    head.distance = length;
                }
            }

            for (size_t i = 1; i < entities.size(); i++) {
                Entity& current = entities[i];
                Entity& ahead = entities[i-1];

                current.distance += speed * dt;
				float spacing = 0.4f;
                // Limit: current entity's front edge can't pass ahead entity's back edge
                // Both entities take up 'size' space, so minimum gap is current.size + ahead.size
                float limit = ahead.distance - ahead.size - current.size - spacing;

                if (current.distance > limit) {
                    current.distance = std::max(0.0f, limit);  // Clamp to non-negative
                }
            }
        }

        float length = 1.0f;
        ConveyorSegment* nextsegment = nullptr;

        std::deque<Entity> entities;
        vector<tx::Coord> WayPoints;

        tx::vec2 p1 = {0, 0}, p2 = {0, 0};
		tx::vec2 center = { 0, 0 };

        // Check if a new entity of given size can enter (entry is at distance 0)
        bool isEntryBlocked(float incomingSize = 0.2f) const {
            if (entities.empty()) return false;
            const Entity& last = entities.back();  // Entity closest to entry point
            // The incoming entity at distance=0 with size would collide if:
            // last.distance - last.size - incomingSize < 0
            return last.distance < last.size + incomingSize;
        }
};






enum class TileType{
    Space,
    Ore_Iron,
    Ore_Copper,
    Ore_Coal,
    Ore_Gold
};

class Tile {
public:

    void update() {
        switch(m_type){

        }
    }

    void setType(TileType in) { m_type = in; }
    // void setExtracter() {}
    void setPos(const tx::Coord& in) { m_pos = in; }
    TileType        type() const { return m_type; }
    const tx::Coord& pos() const {return m_pos;}
    bool operator==(const Tile& other) const { return this->m_type == other.m_type; }
    bool operator!=(const Tile& other) const { return this->m_type != other.m_type; }

    void setConveyor(ConveyorSegment* ptr) {conveyer = ptr; }
    ConveyorSegment* getConveyor() const {return conveyer; }

private:
    TileType m_type = TileType::Space;
    tx::Coord m_pos;
    ConveyorSegment* conveyer = nullptr;
};


// tile amount: 64
class Game {
    using id = uint16_t;
public:
    Game() : 
        rde([](){
            std::random_device rrde;
            std::mt19937 rde_{rrde()};
            return rde_;
        }())
    {
        cout << "start init" << endl;

        tiles.reinit(MapSize);
        tiles.foreach([](Tile& in, const tx::Coord& pos) { in.setPos(pos); });
        
        // Initialize conveyor direction grid with None (no conveyor)
        conveyorDirections.reinit(MapSize);
        conveyorDirections.foreach([](CoordDirection& dir, const tx::Coord&) { dir = CoordDirection::None; });
        
        initJsonObject("./config/config.json", cfg);

        cout << "start init assets..." << endl;
        initAssets();
        cout << "init assets done." << endl;
        genOreTiles_impl();
        initGroundTileMap();
    }

    void update() {
        updateConveyor(0.016f);
    }

    void render(){
        // // tx::Coord cur{0, 0};
        // // for(; cur.y() < MapSize; cur.moveY(1)){
        // //   for(; cur.x() < MapSize; cur.moveX(1)){
        // //       renderTile_impl(cur, tiles.at(cur).type());
        // //   } cur.setX(0);
        // // }

        // tiles.foreach([this](const Tile& tile, const tx::Coord& pos) {
        //  renderTile_impl(pos, tile.type());
        // });

        // // tx::PixelEngine::draw(tiles, [](const Tile& in){
        // //   return tx::getBWColor(!(in.type() == TileType::Space));
        // // });

        // 1. LAYER 1: The Ground (Draw this FIRST so it's at the back)
        renderGroundTiles();

        // 2. LAYER 2: The Resources
        for (id i : ores) {
            renderOres_impl(tiles.atIndex(i));
        }

        // 3. LAYER 3: The Conveyor Belts (Draw these ON TOP of the ground)
        for (const auto& seg : conveyorBelts) {
            // Draw Belt Line
            tx::glColorRGB(tx::RGB(255, 255, 255));
            tx::drawLine(seg.p1, seg.p2);
			tx::drawLine(seg.center, seg.p2);

            // Draw Entities (Items) - smooth interpolation along segment
            for (const auto& entity : seg.entities) {
                float t = entity.distance / seg.length;
                tx::vec2 pos = seg.p1 + (seg.p2 - seg.p1) * t;

				if (t < 0.5f) {
                    // First half: Move from P1 to Center
                    // Map t (0.0 to 0.5) to local (0.0 to 1.0)
                    float localT = t * 2.0f; 
                    pos = seg.p1 + (seg.center - seg.p1) * localT;
                } else {
                    // Second half: Move from Center to P2
                    // Map t (0.5 to 1.0) to local (0.0 to 1.0)
                    float localT = (t - 0.5f) * 2.0f;
                    pos = seg.center + (seg.p2 - seg.center) * localT;
                }

                tx::glColorRGB(tx::RGB(255, 50, 50));
                tx::drawCircle(pos, TileSize * 0.4f);
            }
        }

        // 4. LAYER 4: The Ghost Preview (UI always goes LAST/ON TOP)
        if (isDragging) {
            auto ghostPath = calculatePath(dragStart, dragEnd);
            for (const auto& step : ghostPath) {
                tx::vec2 bottomLeft = getRenderPos(step.pos);
                tx::vec2 topLeft = bottomLeft + tx::vec2{ 0.0f, TileSize };  // Move up to get top-left
                tx::vec2 center = bottomLeft + tx::vec2{ TileSize / 2, TileSize / 2 };

                // Draw Green Box (use drawRectP which takes bottom-left, or use drawRect with top-left)
                tx::glColorRGB(tx::RGB(0, 255, 0));
                tx::drawRectP(bottomLeft, TileSize, TileSize);

                // Draw Direction Line
                tx::Coord d = dirToCoord(step.dir);
                tx::vec2 dirVec = { (float)d.x(), (float)d.y() };
                tx::drawLine(center, center + (dirVec * (TileSize / 2)));
            }
        }
    }



private:
    // runtime data
    std::list<ConveyorSegment> conveyorBelts;
    struct BuildStep {
        tx::Coord pos;
        CoordDirection dir;
    };

    // Grid tracking conveyor directions for each tile (None = no conveyor)
    tx::GridSystem<CoordDirection> conveyorDirections;

    bool isDragging = false;
    tx::Coord dragStart = {0, 0};
    tx::Coord dragEnd = {0, 0};

    tx::GridSystem<Tile> tiles;
    vector<id> ores;

private:
    // config
    tx::JsonObject cfg;
    int MapSize = 64;
    float TileSize = 2.0f / MapSize;
    inline static const tx::KVMap<TileType, string> assetNameMap = {
        {TileType::Ore_Coal,   "coal"},
        {TileType::Ore_Copper, "copper"},
        {TileType::Ore_Gold,   "gold"},
        {TileType::Ore_Iron,   "iron"}
    };
    tx::KVMap<string, vector<id>> assetIndexMap; // { name, vector<index> }
    vector<RGBMap> resources; // all bitmaps
    tx::GridSystem<id> groundTileMap;
private:
    // utility
    std::mt19937 rde;
    std::uniform_int_distribution<int> dist_map{0, MapSize - 1};
    std::uniform_int_distribution<int> dist_np{-1, 1};
    
    void updateConveyor(float dt) {
        for (auto& segment: conveyorBelts) {
            float speed = 2.0f;
            segment.update(dt, speed);
        }
    }

    void setOreTile_impl(const tx::Coord& pos, TileType type) {
        if(!valid_impl(pos)) return;
        tiles.at(pos).setType(type);
        int index = tiles.index(pos);
        ores.push_back(index);
    }



    void genOreTiles_impl() {
        genOre_impl("PolicyCommon", TileType::Ore_Coal);
    }
    void genOre_impl(const string& policy, TileType type) {
        const tx::JsonObject& policyCfg = cfg["OreGeneration"][policy].get<tx::JsonObject>();
        tx::Bitmap circle;
        float radius = [&](){
            float r = policyCfg["radius"].get<float>();
            tx::GridCircle gc{r};
            gc.getBitMap(tx::CoordOrigin, circle);
            return r;
        }();
        tx::Bitmap surroundingCircle;
        float surroundingRadius = [&](){
            float sr = policyCfg["surroundingClusterRadius"].get<float>();
            tx::GridCircle gc{sr};
            gc.getBitMap(tx::CoordOrigin, surroundingCircle);
            return sr;
        }();
        std::uniform_int_distribution<int> dist_offset{
            policyCfg["surroundingClusterOffsetMin"].get<int>(),
            policyCfg["surroundingClusterOffsetMax"].get<int>()};
        
        int clusterAmount = policyCfg["clusterAmount"].get<int>();
        for(int i = 0; i < clusterAmount; ++i){
            tx::Coord center = getRandCoord_impl();
            int surroundingClusterAmount = policyCfg["surroundingClusterAmount"].get<int>();
            //cout << surroundingClusterAmount << endl;
            vector<tx::Coord> surroundingCenters(surroundingClusterAmount);
            for(tx::Coord& i : surroundingCenters){
                i = center.offset(dist_np(rde) * dist_offset(rde), dist_np(rde) * dist_offset(rde));
            }

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
        std::sort(ores.begin(), ores.end());
        ores.erase(std::unique(ores.begin(), ores.end()), ores.end());
    }


    tx::Coord getRandCoord_impl() {
        return tx::Coord{dist_map(rde), dist_map(rde)};
    }
    bool valid_impl(const tx::Coord& in) const {
        return tx::inRange(in, tx::CoordOrigin, tx::Coord{MapSize});
    }



    

    // render ********************************

    // void renderTile_impl(const tx::Coord& in_pos, TileType type){
    //  tx::vec2 pos = tx::toVec2(in_pos) * TileSize;

    //  switch(type){
    //  case TileType::Ore_Coal:
    //  case TileType::Ore_Copper:
    //  case TileType::Ore_Gold:
    //  case TileType::Ore_Iron:
    //      tx::PixelEngine::drawRGBmap(
    //          resources.at(
    //              getRandAsset(assetNameMap.at(type))),
    //          pos, TileSize);
    //      break;
    //  case TileType::Space:

    //      break;
    //  }

    //  //tx::drawRectP(pos, TileSize, TileSize);
        
    // }
    tx::vec2 getRenderPos(const tx::Coord& in) const {
        return tx::toVec2(in) * TileSize - 1.0f;
    }
    void renderOres_impl(const Tile& tile) {
        tx::PixelEngine::drawRGBmap(
            resources.at(getRandAsset(assetNameMap.at(tile.type()))),
            getRenderPos(tile.pos()), TileSize);
    }   


    void initAssets() {
        initAssetIndexMap();
        //loadResources();
    }
    void initAssetIndexMap() {
        const string dirPath = "./converted/";
        const tx::JsonObject& artCfg = cfg["Art"].get<tx::JsonObject>();

        // unique
        tx::KVMap<string, id> pathRecorder; // path : id

        for(const tx::JsonPair& i : artCfg){
            const tx::JsonArray& assetVariants = i.v().get<tx::JsonArray>();
            tx::KVMapHandle hMap = assetIndexMap.insertMulti(i.k());
            hMap.get().reserve(assetVariants.size());
            for(const tx::JsonValue& resourcePath : assetVariants){
                const string& resourcePathStr = resourcePath.get<string>();
                if(!pathRecorder.exist(resourcePathStr)){
                    // resource
                    pathRecorder.insertSingle(resourcePathStr, resources.size());
                    hMap.get().push_back(resources.size());
                    RGBMap bmp = readBMP(dirPath + resourcePathStr);
                    resources.push_back(std::move(bmp));
                    // assetIndexMap
                } else {
                    // assetIndexMap
                    id index = pathRecorder.at(resourcePathStr);
                    hMap.get().push_back(index);
                }               
            }
        } assetIndexMap.validate();
    }
    // void loadResources() {
    //  for(const tx::KVPair<string, vector<string>>& i : assetIndexMap){
    //      for(const string& resourcePathStr : i.v()){
    //          if(resources.exist(resourcePathStr)) continue;
    //          tx::KVMapHandle hMap = resources.insertSingle(resourcePathStr);
    //          RGBMap bmp = readBMP(resourcePathStr);
    //          hMap.get() = std::move(bmp);
    //      }
    //  }
    // }

    // get asset variant of a asset
    id getRandAsset(const string& assetName) {
        const vector<id>& variantPaths = assetIndexMap.at(assetName); // all variant paths for an asset
        std::uniform_int_distribution<int> dist{0, variantPaths.size() - 1};
        return variantPaths[dist(rde)];
    }


    // ground tiles
    
    // must be after all ore gen
    void initGroundTileMap(){
        groundTileMap.reinit(MapSize);
        
        // tiles.foreach([](const Tile& in) {
        //  if(in.type() != TileType::Space){
        //      cout << in.pos() << endl;
        //  }
        // });

        for(id i : ores){ // ores are 1, grass are 0
            groundTileMap.atIndex(i) = 1;
        }
        tx::Bitmap edge; edge.reserve(tx::sq(MapSize) * 0.4);
        groundTileMap.foreach([&](id& in, const tx::Coord& pos) { // find edge
            if(in) return;
            for(uint8_t i = 0; i < 8; ++i){
                tx::Coord adjacent = pos + tx::_8wayIncrement[i];
                if(valid_impl(adjacent) && groundTileMap.at(adjacent)){
                    edge.emplace_back(pos);
                    break;
                }
            }
        });

        groundTileMap.foreach([&](id& in, const tx::Coord& pos) {
            if(in){
                in = getRandAsset("rock");
            } else {
                in = getRandAsset("grass");
            }
        });

        for(const tx::Coord& i : edge){
            groundTileMap.at(i) = assetIndexMap.at("groundEdge")[static_cast<int>(findGroundTileDir(i))];
        }
    }
    CoordDirection findGroundTileDir(const tx::Coord& tile) {
        auto checkNoexcept = [&](const tx::Coord& in) -> bool {
            if(!valid_impl(in)) return 0;
            return groundTileMap.at(in);
        };
        if(checkNoexcept(tile + dirToCoord(CoordDirection::Top   )) && checkNoexcept(tile + dirToCoord(CoordDirection::Left ))) return CoordDirection::TopLeft;
        if(checkNoexcept(tile + dirToCoord(CoordDirection::Top   )) && checkNoexcept(tile + dirToCoord(CoordDirection::Right))) return CoordDirection::TopRight;
        if(checkNoexcept(tile + dirToCoord(CoordDirection::Bottom)) && checkNoexcept(tile + dirToCoord(CoordDirection::Left ))) return CoordDirection::BottomLeft;
        if(checkNoexcept(tile + dirToCoord(CoordDirection::Bottom)) && checkNoexcept(tile + dirToCoord(CoordDirection::Right))) return CoordDirection::BottomRight;
        if(checkNoexcept(tile + dirToCoord(CoordDirection::Top   ))) return CoordDirection::Top;
        if(checkNoexcept(tile + dirToCoord(CoordDirection::Bottom))) return CoordDirection::Bottom;
        if(checkNoexcept(tile + dirToCoord(CoordDirection::Left  ))) return CoordDirection::Left;
        if(checkNoexcept(tile + dirToCoord(CoordDirection::Right ))) return CoordDirection::Right;
    }

    void renderGroundTiles(){
        //cout << resources.size() << endl;
        groundTileMap.foreach([&](id resId, const tx::Coord& pos) {
            //cout << resId << endl;
            tx::PixelEngine::drawRGBmap(
                resources.at(resId),
                getRenderPos(pos), TileSize);
        });
    }


    void initTestConveyors() {
        placeConveyor({2, 2}, CoordDirection::Right);
        placeConveyor({3, 2}, CoordDirection::Right);

        if (tiles.at({2, 2}).getConveyor()) {
            Entity item;
            item.distance = 0.0f;
            item.size = 0.2f;
            item.id = 1;
            tiles.at({2,2}).getConveyor()->entities.push_back(item);
        }
    }

	void placeConveyor(tx::Coord pos, CoordDirection dir) {
        if (!valid_impl(pos)) return;

        // Register direction
        conveyorDirections.at(pos) = dir;

        conveyorBelts.emplace_back();
        ConveyorSegment* newSeg = &conveyorBelts.back();
        newSeg->length = 1.0f;

        // Calculate Geometry
        tx::vec2 tileCorner = getRenderPos(pos);
        float halfSize = TileSize / 2.0f;
        tx::vec2 center = tileCorner + tx::vec2{ halfSize, halfSize };
        newSeg->center = center; // Store center

        tx::Coord delta = dirToCoord(dir);
        tx::vec2 dirVec = { (float)delta.x(), (float)delta.y() };

        // Default: Straight line (Center-Dir -> Center+Dir)
        newSeg->p1 = center - (dirVec * halfSize);
        newSeg->p2 = center + (dirVec * halfSize);

        tiles.at(pos).setConveyor(newSeg);

        // --- 1. BACKWARD SNAP (Inputs) ---
        // Look for neighbors that point AT us. Snap our start to their end.
        for(int i = 0; i < 4; ++i) { // Check NESW
            tx::Coord checkPos = pos + dirToCoord(static_cast<CoordDirection>(i));
            if (!valid_impl(checkPos)) continue;
            
            // Is there a conveyor?
            if (conveyorDirections.at(checkPos) == CoordDirection::None) continue;
            
            ConveyorSegment* prev = tiles.at(checkPos).getConveyor();
            if (!prev) continue;

            // Does it point to us?
            CoordDirection prevDir = conveyorDirections.at(checkPos);
            tx::Coord outputOffset = dirToCoord(prevDir);
            
            if (checkPos + outputOffset == pos) {
                // YES! It feeds us.
                // 1. Snap our Start (p1) to their End (p2)
                newSeg->p1 = prev->p2;
                
                // 2. Link them to us
                prev->nextsegment = newSeg;
                
                // (We break after the first input found to keep it simple)
                break; 
            }
        }

        // --- 2. FORWARD SNAP (Outputs) ---
        // Look at where we are pointing.
        tx::Coord targetPos = pos + delta;
        if (valid_impl(targetPos)) {
            ConveyorSegment* target = tiles.at(targetPos).getConveyor();
            if (target) {
                // We feed them.
                newSeg->nextsegment = target;
                
                // AUTO-CORNER LOGIC:
                // Snap their Start (p1) to our End (p2).
                // This dynamically turns a straight belt into a corner if we side-load it!
                target->p1 = newSeg->p2; 
            }
        }
    }

    std::vector<BuildStep> calculatePath(tx::Coord start, tx::Coord end) {
        std::vector<BuildStep> path;
        int dx = end.x() - start.x();
        int dy = end.y() - start.y();

        tx::Coord current = start;

        int xSteps = std::abs(dx);
        int ySteps = std::abs(dy);
        
        CoordDirection xDir = (dx >= 0) ? CoordDirection::Right : CoordDirection::Left;
        CoordDirection yDir = (dy >= 0) ? CoordDirection::Top : CoordDirection::Bottom;

        // Handle horizontal movement - all horizontal tiles point in the horizontal direction
        for (int i = 0; i < xSteps; i++) {
            path.push_back({current, xDir});
            current = current + dirToCoord(xDir);
        }

        // Handle vertical movement - all vertical tiles point in the vertical direction
        for (int i = 0; i < ySteps; ++i) {
            path.push_back({current, yDir});
            current = current + dirToCoord(yDir);
        }

        // If start == end, place a single tile
        if (path.empty()) {
            path.push_back({start, CoordDirection::Right});
        }

        return path;
    }

public:
    void onMouseEvent(float mouseX, float mouseY, bool isDown, bool isRelease, int windowWidth, int windowHeight) {
        
        // --- 1. CONVERT MOUSE TO NDC (Normalized Device Coordinates) ---
        // OpenGL NDC: X from -1 (left) to +1 (right), Y from -1 (bottom) to +1 (top)
        // Mouse coords: (0,0) at top-left, Y increases downward
        
        // Convert mouse to NDC space (matching what OpenGL renders)
        float ndcX = (mouseX / windowWidth) * 2.0f - 1.0f;   // -1 to +1
        float ndcY = 1.0f - (mouseY / windowHeight) * 2.0f;  // +1 (top) to -1 (bottom), flipped
        
        // --- 2. CONVERT NDC TO GRID ---
        // getRenderPos does: tx::toVec2(in) * TileSize - 1.0f
        // So: ndcPos = gridPos * TileSize - 1.0f
        // Inverse: gridPos = (ndcPos + 1.0f) / TileSize
        
        float gridXf = (ndcX + 1.0f) / TileSize;
        float gridYf = (ndcY + 1.0f) / TileSize;
        
        // Clamp to valid range [0, MapSize-1]
        int gridX = std::clamp((int)std::floor(gridXf), 0, MapSize - 1);
        int gridY = std::clamp((int)std::floor(gridYf), 0, MapSize - 1);
        
        tx::Coord gridPos = { gridX, gridY };

        // --- 3. LOGIC ---
        if (isDown && !isDragging) {
            isDragging = true;
            dragStart = gridPos;
        }

        if (isDragging) {
            dragEnd = gridPos;
        }

        if (isRelease && isDragging) {
            auto path = calculatePath(dragStart, dragEnd);
            for (const auto& step : path) {
                placeConveyor(step.pos, step.dir);
            }
            
            // Spawn an Entity on the first segment of the newly placed conveyor
            if (!path.empty()) {
                ConveyorSegment* firstSeg = tiles.at(path[0].pos).getConveyor();
                if (firstSeg && !firstSeg->isEntryBlocked(0.2f)) {
                    Entity newEntity;
                    newEntity.distance = 0.0f;
                    newEntity.size = 0.2f;
                    newEntity.id = 1;
                    firstSeg->entities.push_back(newEntity);
                }
            }
            
            isDragging = false;
        }
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