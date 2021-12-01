#pragma once

struct Char;
struct Room;
enum class Direction;

void move_char(Char *ch, Direction direction);
void transfer(const Char *imm, Char *victim, Room *location);
