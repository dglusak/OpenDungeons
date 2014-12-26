/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TILE_H
#define TILE_H

#include "entities/GameEntity.h"

#include <OgrePrerequisites.h>
#include <OgreSceneNode.h>
#include <OgreMeshManager.h>

#include <string>
#include <vector>
#include <ostream>
#include <istream>

class Creature;
class Player;
class Room;
class MapLight;
class GameMap;
class CreatureDefinition;
class Trap;
class TreasuryObject;
class ChickenEntity;
class CraftedTrap;
class BuildingObject;
class ODPacket;

/*! \brief The tile class contains information about tile type and contents and is the basic level bulding block.
 *
 * A Tile is the basic building block for the GameMap.  It consists of a tile
 * type (rock, dirt, gold, etc.) as well as a fullness which indicates how much
 * the tile has been dug out.  Additionally the tile contains lists of the
 * entities located within it to aid in AI calculations.
 */
class Tile : public GameEntity
{

friend class TileContainersModificator;
friend class GameMap;
friend class ODServer;

public:
    enum TileType
    {
        nullTileType = 0,
        dirt = 1,
        gold = 2,
        rock = 3,
        water = 4,
        lava = 5,
        claimed = 6
    };

    Tile(GameMap* gameMap, int nX = 0, int nY = 0, TileType nType = dirt, double nFullness = 100.0);

    std::string getOgreNamePrefix() const { return "Tile_"; }

    /*! \brief A mutator to set the type (rock, claimed, etc.) of the tile.
     *
     * In addition to setting the tile type this function also reloads the new mesh
     * for the tile.
     */
    void setType(TileType t);

    //! \brief An accessor which returns the tile type (rock, claimed, etc.).
    TileType getType() const
    {
        return type;
    }

    /*! \brief A mutator to change how "filled in" the tile is.
     *
     * Additionally this function reloads the proper mesh to display to the user
     * how full the tile is.  It also determines the orientation of the
     * tile to make corners display correctly.  Both of these tasks are
     * accomplished by setting the fullnessMeshNumber variable which is
     * concatenated to the tile's type to determine the mesh to load, e.g.
     * Rock104.mesh for a rocky tile which has all 4 sides shown because it is an
     * "island" with all four sides visible.  Claimed102.mesh would be a fully
     * filled in tile but only two sides are drawn because it borders full tiles on
     * 2 sides.
     */
    void setFullness(double f);

    //! \brief An accessor which returns the tile's fullness which should range from 0 to 100.
    double getFullness() const;

    /*! \brief An accessor which returns the tile's fullness mesh number.
     *
     * The fullness mesh number is concatenated to the tile's type to determine the
     * mesh to load to display a given tile type.
     */
    int getFullnessMeshNumber() const;

    //! \brief Tells whether a creature can see through a tile
    bool permitsVision() const;

    /*! \brief This is a helper function to scroll through the list of available fullness levels.
     *
     * This function is used in the map editor when the user presses the button to
     * select the next tile fullness level to be active in the user interface.  The
     * active fullness level is the one which is placed when the user clicks the
     * mouse button.
     */
    static int nextTileFullness(int f);

    //! \brief This is a helper function that generates a mesh filename from a tile type and a fullness mesh number.
    //! \TODO Define what is a postfix.
    static std::string meshNameFromNeighbors(TileType myType, int fullnessMeshNumber, const TileType* neighbors,
                                             const bool* neighborsFullness, int &rt);

    //! \brief Generate the tile mesh name in ss from other parameters.
    //! \param postFixInt an array 0 and 1 set according to neighbor mesh types and fullness.
    //! \param fMN The fullness of the tile (Atm, only tested whether > 0)
    //! \param myType The tile type (dirt, gold, ...)
    //! TODO This will have to be replaced by a proper tileset config file.
    static void meshNameAux(std::stringstream &ss, int &postfixInt, int& fMN, TileType myType);

    //! \brief This function puts a message in the renderQueue to change the mesh for this tile.
    void refreshMesh();

    virtual const Ogre::Vector3& getScale() const
    { return mScale; }

    //! \brief This function marks the tile as being selected through a mouse click or drag.
    void setSelected(bool ss, Player *pp);

    //! \brief This accessor function returns whether or not the tile has been selected.
    bool getSelected() const
    { return selected; }

    bool getIsBuilding() const
    { return mIsBuilding; }

    //! \brief Set the tile digging mark for the given player.
    void setMarkedForDigging(bool s, Player *p);

    //! \brief This accessor function returns whether or not the tile has been marked to be dug out by a given Player p.
    bool getMarkedForDigging(Player *p);

    //! \brief This is a simple helper function which just calls setMarkedForDigging() for everyone in the game except
    //! allied to exceptSeat. If exceptSeat is NULL, it is called for every player
    void setMarkedForDiggingForAllPlayersExcept(bool s, Seat* exceptSeat);

    //! \brief Tells whether the tile is selected for digging by any player/AI.
    bool isMarkedForDiggingByAnySeat();

    //! \brief Add a player to the vector of players who have marked this tile for digging.
    void addPlayerMarkingTile(Player *p);

    void removePlayerMarkingTile(Player *p);
    unsigned numPlayersMarkingTile() const;
    Player* getPlayerMarkingTile(int index);

    //! \brief This function adds an entity to the list of entities in this tile.
    bool addEntity(GameEntity *entity);

    //! \brief This function removes an entity to the list of entities in this tile.
    bool removeEntity(GameEntity *entity);

    //! \brief This function returns the count of the number of creatures in the tile.
    unsigned int numEntitiesInTile() const
    { return mEntitiesInTile.size(); }

    void addNeighbor(Tile *n);Tile* getNeighbor(unsigned index);
    const std::vector<Tile*>& getAllNeighbors() const
    { return mNeighbors; }

    void claimForSeat(Seat* seat, double nDanceRate);
    void claimTile(Seat* seat);
    double digOut(double digRate, bool doScaleDigRate = false);
    double scaleDigRate(double digRate);

    Building* getCoveringBuilding() const
    { return mCoveringBuilding; }
    //! \brief Proxy that checks if there is a covering building and if it is a room. If yes, returns
    //! a pointer to the covering room
    Room* getCoveringRoom() const;
    //! \brief Proxy that checks if there is a covering building and if it is a trap. If yes, returns
    //! a pointer to the covering trap
    Trap* getCoveringTrap() const;

    void setCoveringBuilding(Building *building);
    //! \brief Add a tresaury object in this tile. There can be only one per tile so if there is already one, they are merged
    bool addTreasuryObject(TreasuryObject* object);

    //! \brief Tells whether the tile is diggable by dig-capable creatures.
    //! \brief The player seat.
    //! The function will check whether a tile is not already a reinforced wall owned by another team.
    bool isDiggable(Seat* seat) const;

    //! \brief Tells whether the tile fullness is empty (ground tile) and can be claimed.
    bool isGroundClaimable() const;

    //! \brief Tells whether the tile is a wall (fullness > 1) and can be claimed for the given seat.
    //! Reinforced walls by another team and hard rocks can't be claimed.
    bool isWallClaimable(Seat* seat);
    //! \brief Tells whether the tile is claimed for the given seat.
    bool isClaimedForSeat(Seat* seat) const;

    //! \brief Tells whether the given tile is a claimed wall for the given seat team.
    //! Used to discover active spots for rooms.
    bool isWallClaimedForSeat(Seat* seat);

    //! \brief Tells whether a room can be built upon this tile.
    bool isBuildableUpon() const;

    static const char* getFormat();

    //! \brief Loads the tile data from a level line.
    static void loadFromLine(const std::string& line, Tile *t);

    friend std::ostream& operator<<(std::ostream& os, Tile *t);

    /*! \brief The << operator is used for saving tiles to a file and sending them over the net.
     *
     * This operator is used in conjunction with the >> operator to standardize
     * tile format in the level files, as well as sending tiles over the network.
     */
    friend ODPacket& operator<<(ODPacket& os, Tile *t);

    /*! \brief The >> operator is used for loading tiles from a file and for receiving them over the net.
     *
     * This operator is used in conjunction with the << operator to standardize
     * tile format in the level files, as well as sending tiles over the network.
     */
    friend ODPacket& operator>>(ODPacket& is, Tile *t);

    /*! \brief This is a helper function which just converts the tile type enum into a string.
     *
     * This function is used primarily in forming the mesh names to load from disk
     * for the various tile types.  The name returned by this function is
     * concatenated with a fullnessMeshNumber to form the filename, e.g.
     * Dirt104.mesh is a 4 sided dirt mesh with 100% fullness.
     */
    static std::string tileTypeToString(TileType t);

    int getX() const
    { return x; }

    int getY() const
    { return y; }

    double getClaimedPercentage()
    {
        return mClaimedPercentage;
    }

    static std::string buildName(int x, int y);
    static bool checkTileName(const std::string& tileName, int& x, int& y);

    static std::string displayAsString(Tile* tile);

    int x, y;
    Ogre::Real rotation;

    void doUpkeep(){}
    void receiveExp(double experience){}
    double takeDamage(GameEntity* attacker, double physicalDamage, double magicalDamage, Tile *tileTakingDamage)
    { return 0.0; }
    double getHP(Tile *tile) const {return 0;}
    std::vector<Tile*> getCoveredTiles() { return std::vector<Tile*>() ;}
    void refreshFromTile(const Tile& tile);

    //! \brief Fills entities with all the attackable creatures in the Tile. If invert is true,
    //! the list will be filled with the ennemies with the given seat. If invert is false, it will be filled
    //! with allies with the given seat. For all theses functions, the list is checked to be sure
    //! no entity is added twice
    void fillWithAttackableCreatures(std::vector<GameEntity*>& entities, Seat* seat, bool invert);
    void fillWithAttackableRoom(std::vector<GameEntity*>& entities, Seat* seat, bool invert);
    void fillWithAttackableTrap(std::vector<GameEntity*>& entities, Seat* seat, bool invert);
    void fillWithCarryableEntities(std::vector<MovableGameEntity*>& entities);
    void fillWithChickenEntities(std::vector<GameEntity*>& entities);
    void fillWithCraftedTraps(std::vector<GameEntity*>& entities);

    //! \brief Computes the visible tiles and tags them to know which are visible
    void computeVisibleTiles();
    void clearVision();
    void notifyVision(Seat* seat);

    void setSeats(const std::vector<Seat*>& seats);
    bool hasChangedForSeat(Seat* seat) const;
    void changeNotifiedForSeat(Seat* seat);

    virtual void notifySeatsWithVision();

protected:
    virtual void createMeshLocal();
    virtual void destroyMeshLocal();
private:
    bool isFloodFillFilled();

    enum FloodFillType
    {
        FloodFillTypeGround = 0,
        FloodFillTypeGroundWater,
        FloodFillTypeGroundLava,
        FloodFillTypeGroundWaterLava,
        FloodFillTypeMax
    };

    TileType type;
    bool selected;

    double fullness;
    int fullnessMeshNumber;

    std::vector<Tile*> mNeighbors;
    std::vector<Player*> mPlayersMarkingTile;
    std::vector<std::pair<Seat*, bool>> mTileChangedForSeats;
    std::vector<Seat*> mSeatsWithVision;

    /*! \brief List of the entities actually on this tile. Most of the creatures actions will rely on this list
     */
    std::vector<GameEntity*> mEntitiesInTile;
    Building* mCoveringBuilding;
    int mFloodFillColor[FloodFillTypeMax];
    double mClaimedPercentage;
    Ogre::Vector3 mScale;

    //! true if a building is on this tile. False otherwise. It is used on client side because the clients do not know about
    //! buildings. However, it needs to know the tiles where a building is to display the room/trap costs.
    bool mIsBuilding;

    /*! \brief Set the fullness value for the tile.
     *  This only sets the fullness variable. This function is here to change the value
     *  before a map object has been set. setFullness is called once a map is assigned.
     */
    void setFullnessValue(double f);

    int getFloodFill(FloodFillType type);

    void setDirtyForAllSeats();
};

#endif // TILE_H
