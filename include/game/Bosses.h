#pragma once

#include <memory>

namespace HeavyWeapon {

class Board;
class Craft;

// The boss for a mission (Board::Update 0x41b690 switch on mission % 9), or nullptr for
// bosses that are not ported yet.
std::unique_ptr<Craft> CreateBoss(Board& b, int mission);

} // namespace HeavyWeapon
