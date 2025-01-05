// https://github.com/Jupeyy/teeworlds-fng2-mod/blob/fng_06/src/game/server/laserText.cpp
#include "laser_text.h"
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>

static const bool asciiTable[256][5][3] = {
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 0
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 1
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 2
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 3
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 4
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 5
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 6
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 7
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 8
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 9
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 10
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 11
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 12
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 13
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 14
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 15
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 16
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 17
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 18
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 19
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 20
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 21
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 22
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 23
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 24
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 25
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 26
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 27
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 28
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 29
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 30
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 31
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 32
	{{false, true, false}, {false, true, false}, {false, true, false}, {false, false, false}, {false, true, false}}, // ascii 33 !
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 34
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 35
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 36
	{{true, false, true}, {true, false, false}, {false, true, false}, {false, false, true}, {true, false, true}}, // ascii 37 %
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 38
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 39
	{{false, true, false}, {true, false, false}, {true, false, false}, {true, false, false}, {false, true, false}}, // ascii 4false (
	{{false, true, false}, {false, false, true}, {false, false, true}, {false, false, true}, {false, true, false}}, // ascii 4true )
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 42
	{{false, false, false}, {false, true, false}, {true, true, true}, {false, true, false}, {false, false, false}}, // ascii 43 +
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, true, false}, {true, false, false}}, // ascii 44 ,
	{{false, false, false}, {false, false, false}, {true, true, true}, {false, false, false}, {false, false, false}}, // ascii 45 -
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, true, false}}, // ascii 46 .
	{{false, false, false}, {false, false, true}, {false, true, false}, {true, false, false}, {false, false, false}}, // ascii 47 /
	{{true, true, true}, {true, false, true}, {true, false, true}, {true, false, true}, {true, true, true}}, // false
	{{false, true, false}, {true, true, false}, {false, true, false}, {false, true, false}, {false, true, false}}, // true
	{{true, true, true}, {false, false, true}, {true, true, true}, {true, false, false}, {true, true, true}}, // 2
	{{true, true, true}, {false, false, true}, {true, true, true}, {false, false, true}, {true, true, true}}, // 3
	{{true, false, true}, {true, false, true}, {true, true, true}, {false, false, true}, {false, false, true}}, // 4
	{{true, true, true}, {true, false, false}, {true, true, true}, {false, false, true}, {true, true, true}}, // 5
	{{true, true, true}, {true, false, false}, {true, true, true}, {true, false, true}, {true, true, true}}, // 6
	{{true, true, true}, {false, false, true}, {false, false, true}, {false, false, true}, {false, false, true}}, // 7
	{{true, true, true}, {true, false, true}, {true, true, true}, {true, false, true}, {true, true, true}}, // 8
	{{true, true, true}, {true, false, true}, {true, true, true}, {false, false, true}, {true, true, true}}, // 9
	{{false, false, false}, {false, true, false}, {false, false, false}, {false, true, false}, {false, false, false}}, // :
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 59
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 6false
	{{false, false, false}, {true, true, true}, {false, false, false}, {true, true, true}, {false, false, false}}, // ascii 6true =
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 62
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 63
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 64
	{{true, true, true}, {true, false, true}, {true, true, true}, {true, false, true}, {true, false, true}}, // A
	{{true, false, false}, {true, false, false}, {true, true, true}, {true, false, true}, {true, true, true}}, // B
	{{true, true, true}, {true, false, false}, {true, false, false}, {true, false, false}, {true, true, true}}, // C
	{{false, false, true}, {false, false, true}, {true, true, true}, {true, false, true}, {true, true, true}}, // D
	{{true, true, true}, {true, false, false}, {true, true, true}, {true, false, false}, {true, true, true}}, // E
	{{true, true, true}, {true, false, false}, {true, true, true}, {true, false, false}, {true, false, false}}, // F
	{{true, true, true}, {true, false, true}, {true, true, true}, {false, false, true}, {false, true, true}}, // G
	{{true, false, true}, {true, false, true}, {true, true, true}, {true, false, true}, {true, false, true}}, // H
	{{true, true, true}, {false, true, false}, {false, true, false}, {false, true, false}, {true, true, true}}, // I
	{{false, false, true}, {false, false, true}, {true, false, true}, {true, false, true}, {true, true, true}}, // J
	{{true, false, false}, {true, false, false}, {true, false, true}, {true, true, false}, {true, false, true}}, // K
	{{true, false, false}, {true, false, false}, {true, false, false}, {true, false, false}, {true, true, true}}, // L
	{{true, false, true}, {true, true, true}, {true, true, true}, {true, false, true}, {true, false, true}}, // M
	{{false, false, false}, {false, false, false}, {true, true, true}, {true, false, true}, {true, false, true}}, // N
	{{false, false, false}, {false, false, false}, {true, true, true}, {true, false, true}, {true, true, true}}, // O
	{{true, true, true}, {true, false, true}, {true, true, true}, {true, false, false}, {true, false, false}}, // P
	{{true, true, true}, {true, false, true}, {true, true, true}, {false, false, true}, {false, false, true}}, // Q
	{{false, false, false}, {true, false, true}, {true, true, false}, {true, false, false}, {true, false, false}}, // R
	{{false, true, true}, {true, false, false}, {false, true, false}, {false, false, true}, {true, true, false}}, // S
	{{true, true, true}, {false, true, false}, {false, true, false}, {false, true, false}, {false, true, false}}, // T
	{{false, false, false}, {false, false, false}, {true, false, true}, {true, false, true}, {true, true, true}}, // U
	{{false, false, false}, {false, false, false}, {true, false, true}, {true, true, true}, {false, true, false}}, // V
	{{true, false, true}, {true, false, true}, {true, true, true}, {true, true, true}, {true, true, true}}, // W
	{{false, false, false}, {false, false, false}, {true, false, true}, {false, true, false}, {true, false, true}}, // X
	{{true, false, true}, {true, false, true}, {true, true, true}, {false, true, false}, {false, true, false}}, // Y
	{{true, true, true}, {false, false, true}, {false, true, false}, {true, false, false}, {true, true, true}}, // Z
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 91
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 92
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 93
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 94
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 95
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 96
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 97
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 98
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 99
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 100
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 101
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 102
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 103
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 104
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 105
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 106
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 107
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 108
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 109
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 110
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 111
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 112
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 113
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 114
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 115
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 116
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 117
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 118
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 119
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 120
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 121
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 122
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 123
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 124
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 125
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 126
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 127
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 128
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 129
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 130
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 131
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 132
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 133
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 134
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 135
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 136
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 137
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 138
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 139
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 140
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 141
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 142
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 143
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 144
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 145
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 146
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 147
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 148
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 149
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 150
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 151
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 152
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 153
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 154
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 155
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 156
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 157
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 158
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 159
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 160
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 161
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 162
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 163
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 164
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 165
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 166
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 167
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 168
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 169
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 170
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 171
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 172
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 173
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 174
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 175
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 176
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 177
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 178
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 179
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 180
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 181
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 182
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 183
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 184
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 185
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 186
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 187
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 188
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 189
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 190
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 191
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 192
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 193
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 194
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 195
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 196
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 197
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 198
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 199
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 200
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 201
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 202
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 203
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 204
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 205
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 206
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 207
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 208
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 209
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 210
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 211
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 212
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 213
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 214
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 215
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 216
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 217
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 218
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 219
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 220
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 221
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 222
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 223
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 224
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 225
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 226
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 227
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 228
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 229
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 230
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 231
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 232
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 233
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 234
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 235
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 236
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 237
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 238
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 239
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 240
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 241
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 242
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 243
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 244
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 245
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 246
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 247
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 248
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 249
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 250
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 251
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 252
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 253
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}}, // ascii 254
	{{false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}, {false, false, false}} // ascii 255
};

CLaserText::CLaserText(CGameWorld *pGameWorld, vec2 Pos, int pAliveTicks, const char *pText) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	GameWorld()->InsertEntity(this);

	m_CurTicks = Server()->Tick();
	m_StartTick = Server()->Tick();
	m_AliveTicks = pAliveTicks;

	str_copy(m_aText, pText, sizeof(m_aText));
	m_TextLen = str_length(m_aText);

	m_CharNum = 0;

	for(int i = 0; i < m_TextLen; ++i)
	{
		for(int n = 0; n < 5; ++n)
		{
			for(int j = 0; j < 3; ++j)
			{
				if(asciiTable[static_cast<unsigned char>(m_aText[i])][n][j])
				{
					++m_CharNum;
				}
			}
		}
	}

	m_ppChars = new CLaserChar *[m_CharNum];

	m_PosOffsetCharPoints = 15.0;
	m_PosOffsetChars = m_PosOffsetCharPoints * 3.5;

	int CharCount = 0;
	for(int i = 0; i < m_TextLen; ++i)
	{
		MakeLaser(m_aText[i], i, CharCount);
	}
}

CLaserText::CLaserText(CGameWorld *pGameWorld, vec2 Pos, int AliveTicks, const char *pText, float CharPointOffset, float CharOffsetFactor) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	GameWorld()->InsertEntity(this);

	m_CurTicks = Server()->Tick();
	m_StartTick = Server()->Tick();
	m_AliveTicks = AliveTicks;

	str_copy(m_aText, pText, sizeof(m_aText));
	m_TextLen = str_length(m_aText);

	m_CharNum = 0;

	for(int i = 0; i < m_TextLen; ++i)
	{
		for(int n = 0; n < 5; ++n)
		{
			for(int j = 0; j < 3; ++j)
			{
				if(asciiTable[(unsigned char)m_aText[i]][n][j])
				{
					++m_CharNum;
				}
			}
		}
	}

	m_ppChars = new CLaserChar *[m_CharNum];

	m_PosOffsetCharPoints = CharPointOffset;
	m_PosOffsetChars = m_PosOffsetCharPoints * CharOffsetFactor;

	int CharCount = 0;
	for(int i = 0; i < m_TextLen; ++i)
	{
		MakeLaser(m_aText[i], i, CharCount);
	}
}

CLaserText::~CLaserText()
{
	for(int i = 0; i < m_CharNum; ++i)
	{
		delete m_ppChars[i];
	}
	delete[] m_ppChars;
}

void CLaserText::Reset()
{
	m_MarkedForDestroy = true;
}

void CLaserText::Tick()
{
	if(++m_CurTicks - m_StartTick > m_AliveTicks)
		Reset();
}

void CLaserText::TickPaused()
{
}

static char NeighboursVert(const bool pCharVert[3], int pVertOff)
{
	char Neighbours = 0;
	if(pVertOff > 0)
	{
		if(pCharVert[pVertOff - 1])
			++Neighbours;
	}
	if(pVertOff < 2)
	{
		if(pCharVert[pVertOff + 1])
			++Neighbours;
	}

	return Neighbours;
}

static unsigned char NeighboursHor(const bool aaCharHor[5][3], int HorOff, int VertOff)
{
	char Neighbours = 0;
	if(HorOff > 0)
	{
		if(aaCharHor[HorOff - 1][VertOff])
			++Neighbours;
	}
	if(HorOff < 4)
	{
		if(aaCharHor[HorOff + 1][VertOff])
			++Neighbours;
	}

	return Neighbours;
}

void CLaserText::MakeLaser(char Char, int CharOffset, int &CharCount)
{
	unsigned short aaTail[5][3];
	unsigned char aaNeighbourCount[5][3] = {};

	for(int n = 0; n < 5; ++n)
	{
		for(int j = 0; j < 3; ++j)
		{
			aaTail[n][j] = (asciiTable[static_cast<unsigned char>(Char)][n][j]) ? 0 : static_cast<unsigned short>(-1);
			if(asciiTable[static_cast<unsigned char>(Char)][n][j])
			{
				aaNeighbourCount[n][j] += NeighboursVert(asciiTable[static_cast<unsigned char>(Char)][n], j);
				aaNeighbourCount[n][j] += NeighboursHor(asciiTable[static_cast<unsigned char>(Char)], n, j);
			}
		}
	}

	for(int n = 0; n < 5; ++n)
	{
		for(int j = 0; j < 3; ++j)
		{
			if(!asciiTable[static_cast<unsigned char>(Char)][n][j])
				continue;

			int x = j, y = n;
			int MaxNeighbour = 0;
			bool ForceLine = false;

			for(int d = -1; d <= 1; d += 2)
			{ //  d = -1 (left/up), d = 1 (right/down)
				if(j + d >= 0 && j + d < 3)
				{ // Horizontal neighbors
					if(asciiTable[static_cast<unsigned char>(Char)][n][j + d])
					{
						if(aaTail[n][j + d] != 0 && aaTail[n][j + d] != (n << 8 | (j + d)))
						{
							ForceLine = true;
							aaTail[n][j] = (n << 8 | (j + d));
							x = j + d;
							y = n;
						}
						else if(aaNeighbourCount[n][j + d] > MaxNeighbour)
						{
							MaxNeighbour = aaNeighbourCount[n][j + d];
							x = j + d;
							y = n;
						}
					}
				}
				if(n + d >= 0 && n + d < 5)
				{
					if(asciiTable[static_cast<unsigned char>(Char)][n + d][j])
					{
						if(aaTail[n + d][j] != 0 && aaTail[n + d][j] != ((n + d) << 8 | j))
						{
							ForceLine = true;
							aaTail[n][j] = ((n + d) << 8 | j);
							x = j;
							y = n + d;
						}
						else if(aaNeighbourCount[n + d][j] > MaxNeighbour)
						{
							MaxNeighbour = aaNeighbourCount[n + d][j];
							x = j;
							y = n + d;
						}
					}
				}
			}

			if(!ForceLine)
			{
				aaTail[n][j] = (y << 8 | x);
			}

			CLaserChar *pObj = (m_ppChars[CharCount] = new CLaserChar(GameWorld()));
			pObj->m_Pos.x = m_Pos.x + CharOffset * m_PosOffsetChars + j * m_PosOffsetCharPoints;
			pObj->m_Pos.y = m_Pos.y + n * m_PosOffsetCharPoints;
			pObj->m_FromPos.x = m_Pos.x + CharOffset * m_PosOffsetChars + x * m_PosOffsetCharPoints;
			pObj->m_FromPos.y = m_Pos.y + y * m_PosOffsetCharPoints;

			++CharCount;
		}
	}
}

void CLaserText::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	for(int i = 0; i < m_CharNum; ++i)
	{
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ppChars[i]->GetId(), sizeof(CNetObj_Laser)));
		if(!pObj)
			return;

		pObj->m_X = m_ppChars[i]->m_Pos.x;
		pObj->m_Y = m_ppChars[i]->m_Pos.y;
		pObj->m_FromX = m_ppChars[i]->m_FromPos.x;
		pObj->m_FromY = m_ppChars[i]->m_FromPos.y;
		pObj->m_StartTick = Server()->Tick();
	}
}
