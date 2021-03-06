#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define MAX_TILE_DATA_SIZE 72 // 574 bits
#define BUS_WIDTH 32
#define BYTES_IN_BUS (BUS_WIDTH / 8)
#define MAX_NUM_TOPLEVEL_CALLS (MAX_TILE_DATA_SIZE / BYTES_IN_BUS) // 18

#define BORDER_WIDTH 1
#define CELL_WIDTH (BORDER_WIDTH * 2)

#define BLACK   0b00000000
#define WHITE   0b00000111
#define RED     0b00000100
#define GREEN   0b00000010
#define BLUE    0b00000001
#define CYAN    0b00000011
#define YELLOW  0b00000110
#define MAGENTA 0b00000101

#define VGA_VIEWPORT_WIDTH 800
#define VGA_VIEWPORT_HEIGHT 600
#define BITS_PER_PIXEL 8

#define CORNER '+'
#define CORNER_UART "\u253c"
#define HORIZ_LINE '-'
#define HORIZ_LINE_UART "\u2500"
#define HORIZ_GAP '*'
#define HORIZ_GAP_UART ' '
#define VERT_LINE '|'
#define VERT_LINE_UART "\u2502"
#define VERT_GAP ':'
#define VERT_GAP_UART ' '
#define SPACE ' '
#define RETURN '\r'

#define STACK_SIZE 1024
#define TX_BUF_OFFSET (XPAR_DDR_SDRAM_MPMC_BASEADDR + STACK_SIZE)
#define RX_BUF_OFFSET (TX_BUF_OFFSET + XEL_MAX_FRAME_SIZE)
#define DIRECTIONS_OFFSET (RX_BUF_OFFSET + XEL_MAX_FRAME_SIZE)
#define OPENINGS_OFFSET (DIRECTIONS_OFFSET + DIRECTIONS_ARR_SIZE * 32/8) // LOOK AT ME
#define PRINT_BUF_OFFSET (OPENINGS_OFFSET + OPENINGS_ARR_SIZE)
#define DIRECTIONS_BUF_OFFSET (PRINT_BUF_OFFSET + PRINT_BUF_ARR_SIZE)
#define SOLUTIONS_DIRECTIONS_OFFSET (DIRECTIONS_BUF_OFFSET + DIRECTIONS_BUF_SIZE)
#define VGA_BUF_OFFSET (SOLUTIONS_DIRECTIONS_OFFSET + SOLUTIONS_DIRECTIONS_SIZE)

#define TRUE 1
#define FALSE 0
#define FOREVER 1

#define NORTH 0x8
#define WEST 0x4
#define SOUTH 0x2
#define EAST 0x1

#define NORTH_SOUTH (NORTH | SOUTH) // 1010
#define EAST_WEST (EAST | WEST)     // 0101

#define U32_PER_TILE 32
#define DIRECTIONS_ARR_SIZE 2048
#define OPENINGS_ARR_SIZE 384
#define PRINT_BUF_ARR_SIZE 64
#define DIRECTIONS_BUF_SIZE 256 // 16*16
#define SOLUTIONS_DIRECTIONS_SIZE (MAX_NUM_DIRECTIONS * MAX_MAZE_SIZE * MAX_MAZE_SIZE)
#define FRAME_BUF_SIZE 480000

#define MAX_MAZE_SIZE 8
#define MAX_TILE_SIZE 16
#define MAX_SEED_SIZE 4294967295u
#define MAX_NUM_DIRECTIONS 256
#define MAX_NUM_OPENINGS (MAX_MAZE_SIZE * OPENINGS_BYTES_PER_TILE)
#define DIRECTION_LENGTH 4

#define TILE_REQ_LEN 10
#define SOLUTION_RESP_LEN 12
#define RX_BUF_TILE_DATA_OFFSET 26

#define OPENINGS_BYTES_PER_TILE 6
#define OPENINGS_ROW_SIZE (OPENINGS_BYTES_PER_TILE * MAX_MAZE_SIZE)

#define BITS_PER_CELL 2
#define BITS_PER_LAST_CELL 4

#define ROW_OFFSET_1 0
#define COL_OFFSET_1 1
#define SIDE_OFFSET_1 2
#define ROW_OFFSET_2 3
#define COL_OFFSET_2 4
#define SIDE_OFFSET_2 5

#define DIRECTIONS_END_MARKER 3
#define DIRECTIONS_END_MARKER2 0x30000000
#define DIRECTIONS_END_MARKER_MASK 0x33333333

#endif
