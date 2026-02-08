#define TX_JERRY_IMPL
#include "TXLib/txlib.hpp"
#include "TXLib/txgraphics.hpp"
#include "TXLib/txutility.hpp"
#include "TXLib/txmath.hpp"
#include "TXLib/txmap.hpp"
#include "TXLib/txjson.hpp"

// Utility
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

// --- ASSET LOADING ---
class BMPFile {
public:
    uint16_t bfType;
    uint32_t bfSize;        
    uint16_t bfReserved1;  
    uint16_t bfReserved2;  
    uint32_t bfOffBits;    
    uint32_t dibSize;       
    std::vector<uint8_t> dibData; 
    int32_t width = 0;      
    int32_t height = 0;     
    uint16_t planes = 0;   
    uint16_t bitCount = 0; 
    uint32_t compression = 0;
    bool valid = 1;

    BMPFile(const std::filesystem::path& fp) {
        this->ifs = std::ifstream{fp};
        if (!ifs) {
            valid = 0;
            return;
        }

        // FILE HEADER
        ifs.read(reinterpret_cast<char*>(&bfType), 2);
        ifs.read(reinterpret_cast<char*>(&bfSize), 4);
        ifs.read(reinterpret_cast<char*>(&bfReserved1), 2);
        ifs.read(reinterpret_cast<char*>(&bfReserved2), 2);
        ifs.read(reinterpret_cast<char*>(&bfOffBits), 4);

        if (bfType != 0x4D42) { valid = 0; return; } 

        // DIB HEADER
        ifs.read(reinterpret_cast<char*>(&dibSize), 4);
        if (dibSize < 4) { valid = 0; return; }

        dibData.resize(dibSize - 4);
        ifs.read(reinterpret_cast<char*>(dibData.data()), dibData.size());

        if (dibSize >= 40) {
            std::memcpy(&width,       dibData.data() + 0,  4);
            std::memcpy(&height,      dibData.data() + 4,  4);
            std::memcpy(&planes,      dibData.data() + 8,  2);
            std::memcpy(&bitCount,    dibData.data() + 10, 2);
            std::memcpy(&compression, dibData.data() + 12, 4);
        }
        ifs.seekg(bfOffBits, std::ios::beg);
    }

    std::ifstream& getIfs(){ return ifs; }

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
            map.set(x, y, tx::RGB(r, g, b));
        }
    }
    return map;
}

// --- GAME LOGIC ---

struct Entity {
    float distance = 0.0f;
    float size = 1.0f;
    uint16_t id = 0;
};

class ConveyorSegment {
public:
    void update(float dt, float speed) {
        if (entities.empty()) return;

        Entity& head = entities.front();
        head.distance += speed * dt;

        // Transfer to next segment
        if (head.distance >= length) {
            if (nextsegment && !nextsegment->isEntryBlocked()) {
                Entity transfer = head;
                transfer.distance = 0.0f;
                nextsegment->entities.push_back(transfer); // Push to back (start of next belt)
                entities.pop_front();
                if (entities.empty()) return;
            } else {
                head.distance = length;
            }
        }

        // Collision within segment
        for (size_t i = 1; i < entities.size(); i++) {
            Entity& current = entities[i];
            Entity& ahead = entities[i-1];

            current.distance += speed * dt;
            float spacing = 0.4f;
            float limit = ahead.distance - ahead.size - current.size - spacing;

            if (current.distance > limit) {
                current.distance = std::max(0.0f, limit);
            }
        }
    }

    // Check if a new entity can enter
    bool isEntryBlocked(float incomingSize = 0.2f) const {
        if (entities.empty()) return false;
        const Entity& last = entities.back();
        // Check if the tail of the last entity blocks the entrance
        return last.distance < last.size + incomingSize;
    }

    float length = 1.0f;
    ConveyorSegment* nextsegment = nullptr;
    std::deque<Entity> entities;

    tx::vec2 p1 = {0, 0}, p2 = {0, 0};
    tx::vec2 center = { 0, 0 };
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
    void setType(TileType in) { m_type = in; }
    void setPos(const tx::Coord& in) { m_pos = in; }
    TileType type() const { return m_type; }
    const tx::Coord& pos() const {return m_pos;}
    
    bool operator==(const Tile& other) const { return this->m_type == other.m_type; }
    bool operator!=(const Tile& other) const { return this->m_type != other.m_type; }

    void setConveyor(ConveyorSegment* ptr) { conveyer = ptr; }
    ConveyorSegment* getConveyor() const { return conveyer; }

private:
    TileType m_type = TileType::Space;
    tx::Coord m_pos;
    ConveyorSegment* conveyer = nullptr;
};

// --- GAME CLASS ---
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
        
        // Initialize conveyor direction grid
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
        // 1. LAYER 1: The Ground
        renderGroundTiles();

        // 2. LAYER 2: The Resources
        for (id i : ores) {
            renderOres_impl(tiles.atIndex(i));
        }

        // 3. LAYER 3: The Conveyor Belts
        for (const auto& seg : conveyorBelts) {
            // Debug Lines
            tx::glColorRGB(tx::RGB(255, 255, 255));
            tx::drawLine(seg.p1, seg.p2);
            tx::drawLine(seg.center, seg.p2);

            // Draw Entities (Items)
            for (const auto& entity : seg.entities) {
                float t = entity.distance / seg.length;
                tx::vec2 pos; 
                
                // L-Shape Interpolation
                if (t < 0.5f) {
                    float localT = t * 2.0f; 
                    pos = seg.p1 + (seg.center - seg.p1) * localT;
                } else {
                    float localT = (t - 0.5f) * 2.0f;
                    pos = seg.center + (seg.p2 - seg.center) * localT;
                }

                tx::glColorRGB(tx::RGB(255, 50, 50));
                tx::drawCircle(pos, TileSize * 0.4f);
            }
        }

        // 4. LAYER 4: The Ghost Preview
        if (isDragging) {
            auto ghostPath = calculatePath(dragStart, dragEnd);
            for (const auto& step : ghostPath) {
                tx::vec2 bottomLeft = getRenderPos(step.pos);
                tx::vec2 center = bottomLeft + tx::vec2{ TileSize / 2, TileSize / 2 };

                // Draw Green Box
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

    tx::GridSystem<CoordDirection> conveyorDirections;
    bool isDragging = false;
    tx::Coord dragStart = {0, 0};
    tx::Coord dragEnd = {0, 0};

    tx::GridSystem<Tile> tiles;
    vector<id> ores;

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
    tx::KVMap<string, vector<id>> assetIndexMap; 
    vector<RGBMap> resources; 
    tx::GridSystem<id> groundTileMap;
    
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
    }

    void initAssetIndexMap() {
        const string dirPath = "./converted/";
        const tx::JsonObject& artCfg = cfg["Art"].get<tx::JsonObject>();

        tx::KVMap<string, id> pathRecorder; 

        for(const tx::JsonPair& i : artCfg){
            const tx::JsonArray& assetVariants = i.v().get<tx::JsonArray>();
            tx::KVMapHandle hMap = assetIndexMap.insertMulti(i.k());
            hMap.get().reserve(assetVariants.size());
            for(const tx::JsonValue& resourcePath : assetVariants){
                const string& resourcePathStr = resourcePath.get<string>();
                if(!pathRecorder.exist(resourcePathStr)){
                    pathRecorder.insertSingle(resourcePathStr, resources.size());
                    hMap.get().push_back(resources.size());
                    RGBMap bmp = readBMP(dirPath + resourcePathStr);
                    resources.push_back(std::move(bmp));
                } else {
                    id index = pathRecorder.at(resourcePathStr);
                    hMap.get().push_back(index);
                }               
            }
        } assetIndexMap.validate();
    }

    id getRandAsset(const string& assetName) {
        const vector<id>& variantPaths = assetIndexMap.at(assetName);
        std::uniform_int_distribution<int> dist{0, (int)variantPaths.size() - 1};
        return variantPaths[dist(rde)];
    }

    void initGroundTileMap(){
        groundTileMap.reinit(MapSize);
        for(id i : ores){ groundTileMap.atIndex(i) = 1; } // ores are 1, grass are 0
        
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
            if(in){ in = getRandAsset("rock"); } 
            else { in = getRandAsset("grass"); }
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
        return CoordDirection::Right; // Default
    }

    void renderGroundTiles(){
        groundTileMap.foreach([&](id resId, const tx::Coord& pos) {
            tx::PixelEngine::drawRGBmap(resources.at(resId), getRenderPos(pos), TileSize);
        });
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
        newSeg->center = center; 

        tx::Coord delta = dirToCoord(dir);
        tx::vec2 dirVec = { (float)delta.x(), (float)delta.y() };

        // Default: Straight line (Center-Dir -> Center+Dir)
        newSeg->p1 = center - (dirVec * halfSize);
        newSeg->p2 = center + (dirVec * halfSize);

        tiles.at(pos).setConveyor(newSeg);

        // --- 1. BACKWARD SNAP (Inputs) ---
        for(int i = 0; i < 4; ++i) { // Check NESW
            tx::Coord checkPos = pos + dirToCoord(static_cast<CoordDirection>(i));
            if (!valid_impl(checkPos)) continue;
            
            if (conveyorDirections.at(checkPos) == CoordDirection::None) continue;
            ConveyorSegment* prev = tiles.at(checkPos).getConveyor();
            if (!prev) continue;

            CoordDirection prevDir = conveyorDirections.at(checkPos);
            tx::Coord outputOffset = dirToCoord(prevDir);
            
            if (checkPos + outputOffset == pos) {
                newSeg->p1 = prev->p2;
                prev->nextsegment = newSeg;
                break; 
            }
        }

        // --- 2. FORWARD SNAP (Outputs) ---
        tx::Coord targetPos = pos + delta;
        if (valid_impl(targetPos)) {
            ConveyorSegment* target = tiles.at(targetPos).getConveyor();
            if (target) {
                newSeg->nextsegment = target;
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

        for (int i = 0; i < xSteps; i++) {
            path.push_back({current, xDir});
            current = current + dirToCoord(xDir);
        }

        for (int i = 0; i < ySteps; ++i) {
            path.push_back({current, yDir});
            current = current + dirToCoord(yDir);
        }

        if (path.empty()) {
            path.push_back({start, CoordDirection::Right});
        }
        return path;
    }

public:
    void onMouseEvent(float mouseX, float mouseY, bool isDown, bool isRelease, int windowWidth, int windowHeight) {
        // 1. CALCULATE VIEWPORT
        float ndcX = (mouseX / windowWidth) * 2.0f - 1.0f;   
        float ndcY = 1.0f - (mouseY / windowHeight) * 2.0f;
        
        // 2. CONVERT NDC TO GRID
        float gridXf = (ndcX + 1.0f) / TileSize;
        float gridYf = (ndcY + 1.0f) / TileSize;
        
        int gridX = std::clamp((int)std::floor(gridXf), 0, MapSize - 1);
        int gridY = std::clamp((int)std::floor(gridYf), 0, MapSize - 1);
        tx::Coord gridPos = { gridX, gridY };

        // 3. LOGIC
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
            
            // Spawn test Entity
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