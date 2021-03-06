#include "hlt/game.hpp"
#include "hlt/constants.hpp"
#include "hlt/log.hpp"
#include "hlt/shipType.cpp"
#include <random>
#include <ctime>
#include "hlt/addedMoves.hpp"

using namespace std;
using namespace hlt;

int main(int argc, char* argv[])
{
    unsigned int rng_seed;
    if (argc > 1) {
        rng_seed = static_cast<unsigned int>(stoul(argv[1]));
    }
    else {
        rng_seed = static_cast<unsigned int>(time(nullptr));
    }
    mt19937 rng(rng_seed);

    Game game;
    // At this point "game" variable is populated with initial map data.
    // This is a good place to do computationally expensive start-up pre-processing.
    // As soon as you call "ready" function below, the 2 second per turn timer will start.
    game.ready("MyCppBot");

    log::log("Successfully created bot! My Player ID is " + to_string(game.my_id) + ". Bot rng seed is " + to_string(rng_seed) + ".");

    //Initializing our updates
    //unordered_map<shared_ptr<Ship>, ShipType> typedShips; //Contains all kinds of ship identified by index
    unordered_map<EntityId, ShipType> typedShips; //Contains all kinds of ship identified by index
    AddedMoves addedMoves; //Calls all additionnal moves
    ShipType lastSpawnedShip = Nothing;

    for (;;) {
        game.update_frame();
        shared_ptr<Player> me = game.me;
        unique_ptr<GameMap>& game_map = game.game_map;

        vector<Command> command_queue;

        int shipsCount = me->ships.size();

        if (lastSpawnedShip != Nothing)
        {
            shared_ptr<Ship>& ship = prev(me->ships.end())->second;

            pair<EntityId, ShipType> typedShip(ship->id, lastSpawnedShip);

            if (typedShips.find(ship->id) == typedShips.end())
            {
                log::log("new bot spawned");
                typedShips.insert(typedShip);
            }
            lastSpawnedShip = Nothing;
        }

        for (auto& type_iterator : typedShips) // For all kinds of ship
        {
            Command move;
            log::log(to_string(type_iterator.second));
            auto ship_it = me->ships.find(type_iterator.first); //get the corresponding ship

            if (ship_it != me->ships.end()) // If the ship exist
            {
                auto& ship = ship_it->second; //use it

                if (game_map->at(ship)->halite < constants::MAX_HALITE / 10 || ship->is_full())
                {
                    switch (type_iterator.second)
                    {
                        case Scout : // If the ship is a scout
                            if (ship->position != addedMoves.treasurePosition)
                            {
                                move = addedMoves.moveToTreasure(ship, game_map); //it will find the cell with the most halite
                            }
                            else
                            {
                                type_iterator.second = Beacon;
                                move = ship->stay_still();
                            }
                            break;
                        case Beacon: // If the ship is a beacon
                            if(game_map->at(addedMoves.treasurePosition) == 0)
                            {
                                //addedMoves.treasurePosition.x = 0;
                                type_iterator.second = Picker; //it becomes a picker
                            }
                            move = ship->stay_still();
                            break;
                    }
                }

                if (find(command_queue.begin(), command_queue.end(), move) == command_queue.end())
                {
                    command_queue.push_back(move);
                }
            }
            else
            {
                typedShips.erase(type_iterator.first);
            }
        }

        if (game.turn_number <= constants::MAX_TURNS && me->halite >= constants::SHIP_COST && !game_map->at(me->shipyard)->is_occupied())
        {
            if (shipsCount == 0)
            {
                command_queue.push_back(me->shipyard->spawn());
                lastSpawnedShip = Scout;
            }
            else if (addedMoves.treasurePosition.x != NULL)
            {
                command_queue.push_back(me->shipyard->spawn());
                lastSpawnedShip = Picker;
            }
        }

        if (!game.end_turn(command_queue)) {
            break;
        }
    }

    return 0;
}