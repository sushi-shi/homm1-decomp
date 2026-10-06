#include <H1/Ints.h>

#include <BASE/LZHUF.h>

#include <BASE/LZHUF_internal.h>
#include <BASE/Misc.h>
void InitializeTree(void);
void ReconstructEncoderTree(void);
static void EncodeEnd(void);
#include <SOURCE/KB.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_SIZE 4096
#define LOOK_AHEAD 60
#define MATCH_THRESHOLD 2
#define NIL WINDOW_SIZE
#define CHARACTER_COUNT (256 - MATCH_THRESHOLD + LOOK_AHEAD)
#define TREE_SIZE (CHARACTER_COUNT * 2 - 1)
#define ROOT (TREE_SIZE - 1)
#define MAX_FREQUENCY 0x8000

extern "C" {
i16 match_position;
i16 prnt[TREE_SIZE + CHARACTER_COUNT];
i16 son[TREE_SIZE];
FILE* infile;
u16 getbuf;
u8 getlen;
u8 text_buf[WINDOW_SIZE + LOOK_AHEAD - 1];
i32 gLzhufOldPad;
u16 freq[TREE_SIZE + 1];
i16 match_length;
i16 lson[WINDOW_SIZE + 1];
i32 gLzhufOldId;
i16 rson[WINDOW_SIZE + 257];
i16 dad[WINDOW_SIZE + 1];
i32 gOldLzhufKeyPos;
u32 textsize;
u32 codesize;
i32 gOldLzhufPtr;
u16 putbuf;
u8 putlen;
char *dataPtr;
char *outputPos;
u32 decodeLen;
u8 d_code[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7, 8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,
    10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,
    12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15,
    16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19,
    20,20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,
    24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,
    32,32,33,33,34,34,35,35,36,36,37,37,38,38,39,39,
    40,40,41,41,42,42,43,43,44,44,45,45,46,46,47,47,
    48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63
};

u8 d_len[256] = {
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4, 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5, 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
};

i16 initialSon[627] = {
    627, 628, 629, 630, 631, 632, 633, 634, 635, 636, 637, 638,
    639, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649, 650,
    651, 652, 653, 654, 655, 656, 657, 658, 659, 660, 661, 662,
    663, 664, 665, 666, 667, 668, 669, 670, 671, 672, 673, 674,
    675, 676, 677, 678, 679, 680, 681, 682, 683, 684, 685, 686,
    687, 688, 689, 690, 691, 692, 693, 694, 695, 696, 697, 698,
    699, 700, 701, 702, 703, 704, 705, 706, 707, 708, 709, 710,
    711, 712, 713, 714, 715, 716, 717, 718, 719, 720, 721, 722,
    723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 734,
    735, 736, 737, 738, 739, 740, 741, 742, 743, 744, 745, 746,
    747, 748, 749, 750, 751, 752, 753, 754, 755, 756, 757, 758,
    759, 760, 761, 762, 763, 764, 765, 766, 767, 768, 769, 770,
    771, 772, 773, 774, 775, 776, 777, 778, 779, 780, 781, 782,
    783, 784, 785, 786, 787, 788, 789, 790, 791, 792, 793, 794,
    795, 796, 797, 798, 799, 800, 801, 802, 803, 804, 805, 806,
    807, 808, 809, 810, 811, 812, 813, 814, 815, 816, 817, 818,
    819, 820, 821, 822, 823, 824, 825, 826, 827, 828, 829, 830,
    831, 832, 833, 834, 835, 836, 837, 838, 839, 840, 841, 842,
    843, 844, 845, 846, 847, 848, 849, 850, 851, 852, 853, 854,
    855, 856, 857, 858, 859, 860, 861, 862, 863, 864, 865, 866,
    867, 868, 869, 870, 871, 872, 873, 874, 875, 876, 877, 878,
    879, 880, 881, 882, 883, 884, 885, 886, 887, 888, 889, 890,
    891, 892, 893, 894, 895, 896, 897, 898, 899, 900, 901, 902,
    903, 904, 905, 906, 907, 908, 909, 910, 911, 912, 913, 914,
    915, 916, 917, 918, 919, 920, 921, 922, 923, 924, 925, 926,
    927, 928, 929, 930, 931, 932, 933, 934, 935, 936, 937, 938,
    939, 940, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18,
    20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42,
    44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66,
    68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90,
    92, 94, 96, 98, 100, 102, 104, 106, 108, 110, 112, 114,
    116, 118, 120, 122, 124, 126, 128, 130, 132, 134, 136, 138,
    140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162,
    164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186,
    188, 190, 192, 194, 196, 198, 200, 202, 204, 206, 208, 210,
    212, 214, 216, 218, 220, 222, 224, 226, 228, 230, 232, 234,
    236, 238, 240, 242, 244, 246, 248, 250, 252, 254, 256, 258,
    260, 262, 264, 266, 268, 270, 272, 274, 276, 278, 280, 282,
    284, 286, 288, 290, 292, 294, 296, 298, 300, 302, 304, 306,
    308, 310, 312, 314, 316, 318, 320, 322, 324, 326, 328, 330,
    332, 334, 336, 338, 340, 342, 344, 346, 348, 350, 352, 354,
    356, 358, 360, 362, 364, 366, 368, 370, 372, 374, 376, 378,
    380, 382, 384, 386, 388, 390, 392, 394, 396, 398, 400, 402,
    404, 406, 408, 410, 412, 414, 416, 418, 420, 422, 424, 426,
    428, 430, 432, 434, 436, 438, 440, 442, 444, 446, 448, 450,
    452, 454, 456, 458, 460, 462, 464, 466, 468, 470, 472, 474,
    476, 478, 480, 482, 484, 486, 488, 490, 492, 494, 496, 498,
    500, 502, 504, 506, 508, 510, 512, 514, 516, 518, 520, 522,
    524, 526, 528, 530, 532, 534, 536, 538, 540, 542, 544, 546,
    548, 550, 552, 554, 556, 558, 560, 562, 564, 566, 568, 570,
    572, 574, 576, 578, 580, 582, 584, 586, 588, 590, 592, 594,
    596, 598, 600, 602, 604, 606, 608, 610, 612, 614, 616, 618,
    620, 622, 624,
};
i16 initialParent[941] = {
    314, 314, 315, 315, 316, 316, 317, 317, 318, 318, 319, 319,
    320, 320, 321, 321, 322, 322, 323, 323, 324, 324, 325, 325,
    326, 326, 327, 327, 328, 328, 329, 329, 330, 330, 331, 331,
    332, 332, 333, 333, 334, 334, 335, 335, 336, 336, 337, 337,
    338, 338, 339, 339, 340, 340, 341, 341, 342, 342, 343, 343,
    344, 344, 345, 345, 346, 346, 347, 347, 348, 348, 349, 349,
    350, 350, 351, 351, 352, 352, 353, 353, 354, 354, 355, 355,
    356, 356, 357, 357, 358, 358, 359, 359, 360, 360, 361, 361,
    362, 362, 363, 363, 364, 364, 365, 365, 366, 366, 367, 367,
    368, 368, 369, 369, 370, 370, 371, 371, 372, 372, 373, 373,
    374, 374, 375, 375, 376, 376, 377, 377, 378, 378, 379, 379,
    380, 380, 381, 381, 382, 382, 383, 383, 384, 384, 385, 385,
    386, 386, 387, 387, 388, 388, 389, 389, 390, 390, 391, 391,
    392, 392, 393, 393, 394, 394, 395, 395, 396, 396, 397, 397,
    398, 398, 399, 399, 400, 400, 401, 401, 402, 402, 403, 403,
    404, 404, 405, 405, 406, 406, 407, 407, 408, 408, 409, 409,
    410, 410, 411, 411, 412, 412, 413, 413, 414, 414, 415, 415,
    416, 416, 417, 417, 418, 418, 419, 419, 420, 420, 421, 421,
    422, 422, 423, 423, 424, 424, 425, 425, 426, 426, 427, 427,
    428, 428, 429, 429, 430, 430, 431, 431, 432, 432, 433, 433,
    434, 434, 435, 435, 436, 436, 437, 437, 438, 438, 439, 439,
    440, 440, 441, 441, 442, 442, 443, 443, 444, 444, 445, 445,
    446, 446, 447, 447, 448, 448, 449, 449, 450, 450, 451, 451,
    452, 452, 453, 453, 454, 454, 455, 455, 456, 456, 457, 457,
    458, 458, 459, 459, 460, 460, 461, 461, 462, 462, 463, 463,
    464, 464, 465, 465, 466, 466, 467, 467, 468, 468, 469, 469,
    470, 470, 471, 471, 472, 472, 473, 473, 474, 474, 475, 475,
    476, 476, 477, 477, 478, 478, 479, 479, 480, 480, 481, 481,
    482, 482, 483, 483, 484, 484, 485, 485, 486, 486, 487, 487,
    488, 488, 489, 489, 490, 490, 491, 491, 492, 492, 493, 493,
    494, 494, 495, 495, 496, 496, 497, 497, 498, 498, 499, 499,
    500, 500, 501, 501, 502, 502, 503, 503, 504, 504, 505, 505,
    506, 506, 507, 507, 508, 508, 509, 509, 510, 510, 511, 511,
    512, 512, 513, 513, 514, 514, 515, 515, 516, 516, 517, 517,
    518, 518, 519, 519, 520, 520, 521, 521, 522, 522, 523, 523,
    524, 524, 525, 525, 526, 526, 527, 527, 528, 528, 529, 529,
    530, 530, 531, 531, 532, 532, 533, 533, 534, 534, 535, 535,
    536, 536, 537, 537, 538, 538, 539, 539, 540, 540, 541, 541,
    542, 542, 543, 543, 544, 544, 545, 545, 546, 546, 547, 547,
    548, 548, 549, 549, 550, 550, 551, 551, 552, 552, 553, 553,
    554, 554, 555, 555, 556, 556, 557, 557, 558, 558, 559, 559,
    560, 560, 561, 561, 562, 562, 563, 563, 564, 564, 565, 565,
    566, 566, 567, 567, 568, 568, 569, 569, 570, 570, 571, 571,
    572, 572, 573, 573, 574, 574, 575, 575, 576, 576, 577, 577,
    578, 578, 579, 579, 580, 580, 581, 581, 582, 582, 583, 583,
    584, 584, 585, 585, 586, 586, 587, 587, 588, 588, 589, 589,
    590, 590, 591, 591, 592, 592, 593, 593, 594, 594, 595, 595,
    596, 596, 597, 597, 598, 598, 599, 599, 600, 600, 601, 601,
    602, 602, 603, 603, 604, 604, 605, 605, 606, 606, 607, 607,
    608, 608, 609, 609, 610, 610, 611, 611, 612, 612, 613, 613,
    614, 614, 615, 615, 616, 616, 617, 617, 618, 618, 619, 619,
    620, 620, 621, 621, 622, 622, 623, 623, 624, 624, 625, 625,
    626, 626, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
    69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92,
    93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
    117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,
    129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140,
    141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152,
    153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176,
    177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
    189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
    201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
    213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236,
    237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248,
    249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260,
    261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272,
    273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284,
    285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296,
    297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308,
    309, 310, 311, 312, 313,
};
u16 initialFrequency[628] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    10, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 26, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 58, 64, 64, 64, 64, 122,
    128, 186, 314, 65535,
};
}

static u8 positionLength[64] = {
    0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};

static u8 positionCode[64] = {
    0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
    0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
    0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
    0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
    0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
    0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

i32 DecodeData(char *destination, char *source)
{
    register u32 size;

    textsize = 0;
    dataPtr = source;
    size = static_cast<u8>(*dataPtr++);
    size <<= 8;
    size |= static_cast<u8>(*dataPtr++);
    size <<= 8;
    size |= static_cast<u8>(*dataPtr++);
    size <<= 8;
    size |= static_cast<u8>(*dataPtr++);
    getbuf = getlen = 0;
    memcpy(son, initialSon, sizeof(son));
    memcpy(freq, initialFrequency, sizeof(freq));
    memcpy(prnt, initialParent, sizeof(prnt));
    // The window starts as the encoder's does, filled with spaces: the
    // original left it as the last EncodeData had (or zero), so a stream that
    // referred to the initial spaces decoded to other bytes.
    memset(text_buf, ' ', WINDOW_SIZE - LOOK_AHEAD);
    decodeLen = size;
    outputPos = destination;
    Decode();
    i32 decodedSize = static_cast<i32>(size);
    return decodedSize;
}

i32 EncodeData(char *destination, char *source, u32 sourceLength)
{
    register i16 i, c, r, s, last_match_length;
    register u16 len;
    u32 stillConsumed;

    putbuf = putlen = getbuf = getlen = codesize = 0;
    memset(freq, 0, sizeof(freq));
    memset(prnt, 0, sizeof(prnt));
    memset(son, 0, sizeof(son));

    dataPtr = destination;
    *dataPtr++ = static_cast<char>((sourceLength & 0xff000000) >> 24);
    *dataPtr++ = static_cast<char>((sourceLength & 0x00ff0000) >> 16);
    *dataPtr++ = static_cast<char>((sourceLength & 0x0000ff00) >> 8);
    *dataPtr++ = static_cast<char>(sourceLength & 0xff);

    stillConsumed = 0;
    memcpy(son, initialSon, sizeof(son));
    memcpy(freq, initialFrequency, sizeof(freq));
    memcpy(prnt, initialParent, sizeof(prnt));
    InitializeTree();
    s = 0;
    r = WINDOW_SIZE - LOOK_AHEAD;
    for (i = s; i < r; ++i)
        text_buf[i] = ' ';
    for (len = 0; len < LOOK_AHEAD && stillConsumed < sourceLength;
         ++len, ++source, ++stillConsumed)
        text_buf[r + len] = *source;

    for (i = 1; i <= LOOK_AHEAD; ++i)
        InsertNode(r - i);
    InsertNode(r);
    do {
        if (match_length > len)
            match_length = static_cast<i16>(len);
        if (match_length <= MATCH_THRESHOLD) {
            match_length = 1;
            EncodeCharacter(text_buf[r]);
        } else {
            EncodeCharacter(255 - MATCH_THRESHOLD + match_length);
            EncodePosition(match_position);
        }
        last_match_length = match_length;
        for (i = 0; i < last_match_length && stillConsumed < sourceLength;
             ++i, ++stillConsumed, ++source) {
            c = *source;
            DeleteNode(s);
            text_buf[s] = static_cast<u8>(c);
            if (s < LOOK_AHEAD - 1)
                text_buf[s + WINDOW_SIZE] = static_cast<u8>(c);
            s = (s + 1) & (WINDOW_SIZE - 1);
            r = (r + 1) & (WINDOW_SIZE - 1);
            InsertNode(r);
        }
        while (i++ < last_match_length) {
            DeleteNode(s);
            s = (s + 1) & (WINDOW_SIZE - 1);
            r = (r + 1) & (WINDOW_SIZE - 1);
            if (--len != 0)
                InsertNode(r);
        }
        PollSound();
    } while (len > 0);

    EncodeEnd();
    return static_cast<i32>(codesize);
}

void InitializeTree(void)
{
    i16 i;
    for (i = WINDOW_SIZE + 1; i <= WINDOW_SIZE + 256; ++i)
        rson[i] = NIL;
    for (i = 0; i < WINDOW_SIZE; ++i)
        dad[i] = NIL;
}

static void InsertNode(i16 node)
{
    register u8 *key;
    register i16 cmp, i, p;
    u16 c;

    cmp = 1;
    key = &text_buf[node];
    p = WINDOW_SIZE + 1 + key[0];
    rson[node] = lson[node] = NIL;
    match_length = 0;
    for (;;) {
        if (cmp >= 0) {
            if (rson[p] != NIL)
                p = rson[p];
            else {
                rson[p] = static_cast<i16>(node);
                dad[node] = static_cast<i16>(p);
                return;
            }
        } else {
            if (lson[p] != NIL)
                p = lson[p];
            else {
                lson[p] = static_cast<i16>(node);
                dad[node] = static_cast<i16>(p);
                return;
            }
        }
        for (i = 1; i < LOOK_AHEAD; ++i) {
            cmp = key[i] - text_buf[p + i];
            if (cmp != 0)
                break;
        }
        if (i > MATCH_THRESHOLD) {
            if (i > match_length) {
                match_position = static_cast<i16>(
                    ((node - p) & (WINDOW_SIZE - 1)) - 1);
                if ((match_length = i) >= LOOK_AHEAD)
                    break;
            }
            if (i == match_length)
                if (static_cast<i32>(c = static_cast<u16>(
                        ((node - p) & (WINDOW_SIZE - 1)) - 1)) <
                    match_position)
                    match_position = static_cast<i16>(c);
        }
    }
    dad[node] = dad[p];
    lson[node] = lson[p];
    rson[node] = rson[p];
    dad[lson[p]] = static_cast<i16>(node);
    dad[rson[p]] = static_cast<i16>(node);
    if (rson[dad[p]] == p)
        rson[dad[p]] = static_cast<i16>(node);
    else
        lson[dad[p]] = static_cast<i16>(node);
    dad[p] = NIL;
}

static void DeleteNode(i16 node)
{
    i16 replacement;

    if (dad[node] == NIL)
        return;
    if (rson[node] == NIL)
        replacement = lson[node];
    else if (lson[node] == NIL)
        replacement = rson[node];
    else {
        replacement = lson[node];
        if (rson[replacement] != NIL) {
            do {
                replacement = rson[replacement];
            } while (rson[replacement] != NIL);
            rson[dad[replacement]] = lson[replacement];
            dad[lson[replacement]] = dad[replacement];
            lson[replacement] = lson[node];
            dad[lson[node]] = static_cast<i16>(replacement);
        }
        rson[replacement] = rson[node];
        dad[rson[node]] = static_cast<i16>(replacement);
    }
    dad[replacement] = dad[node];
    if (rson[dad[node]] == node)
        rson[dad[node]] = static_cast<i16>(replacement);
    else
        lson[dad[node]] = static_cast<i16>(replacement);
    dad[node] = NIL;
}

void EncodeCharacter(u16 character)
{
    u16 code, len;
    u16 i;
    i16 j, k;

    i = 0;
    j = 0;
    k = prnt[character + TREE_SIZE];
    do {
        i >>= 1;
        if (k & 1)
            i += 0x8000;
        ++j;
        k = prnt[k];
    } while (k != ROOT);
    PutCode(j, i);
    code = i;
    len = j;
    UpdateEncoderTree(character);
}

static void UpdateEncoderTree(i16 character)
{
    i16 k;
    register i16 i, j, l;

    if (freq[ROOT] == MAX_FREQUENCY)
        ReconstructEncoderTree();
    character = prnt[character + TREE_SIZE];
    do {
        k = ++freq[character];
        if (k > freq[l = character + 1]) {
            while (k > freq[++l])
                ;
            --l;
            freq[character] = freq[l];
            freq[l] = k;

            i = son[character];
            prnt[i] = static_cast<i16>(l);
            if (i < TREE_SIZE)
                prnt[i + 1] = static_cast<i16>(l);
            j = son[l];
            son[l] = static_cast<i16>(i);
            prnt[j] = static_cast<i16>(character);
            if (j < TREE_SIZE)
                prnt[j + 1] = static_cast<i16>(character);
            son[character] = static_cast<i16>(j);
            character = l;
        }
        character = prnt[character];
    } while (character != 0);
}

void ReconstructEncoderTree(void)
{
    i16 k, i, j;
    u16 f, l;

    j = 0;
    for (i = 0; i < TREE_SIZE; ++i) {
        if (son[i] >= TREE_SIZE) {
            freq[j] = static_cast<u16>((freq[i] + 1) / 2);
            son[j] = son[i];
            ++j;
        }
    }
    for (i = 0, j = CHARACTER_COUNT; j < TREE_SIZE; i += 2, ++j) {
        f = freq[j] = static_cast<u16>(freq[i] + freq[i + 1]);
        for (k = j - 1; f < freq[k]; --k)
            ;
        ++k;
        l = static_cast<u16>((j - k) * 2);
        memmove(&freq[k + 1], &freq[k], l);
        freq[k] = f;
        memmove(&son[k + 1], &son[k], l);
        son[k] = static_cast<i16>(i);
    }
    for (i = 0; i < TREE_SIZE; ++i) {
        k = son[i];
        if (k >= TREE_SIZE)
            prnt[k] = static_cast<i16>(i);
        else
            prnt[k] = prnt[k + 1] = static_cast<i16>(i);
    }
}

void PutCode(i16 length, u16 code)
{
    putbuf |= code >> putlen;
    putlen += length;
    if (putlen >= 8) {
        *dataPtr++ = static_cast<char>(putbuf >> 8) & 0xff;
        putlen -= 8;
        if (putlen >= 8) {
            *dataPtr++ = static_cast<char>(putbuf) & 0xff;
            codesize += 2;
            putlen -= 8;
            putbuf = static_cast<u16>(code << (length - putlen));
        } else {
            putbuf <<= 8;
            ++codesize;
        }
    }
}

void EncodePosition(u16 position)
{
    u16 upper;

    upper = position >> 6;
    PutCode(positionLength[upper],
            static_cast<u16>(positionCode[upper] << 8));
    PutCode(6, static_cast<u16>((position & 0x3F) << 10));
}

static void EncodeEnd(void)
{
    if (putlen != 0) {
        *dataPtr++ = static_cast<char>(putbuf >> 8) & 0xff;
        ++codesize;
    }
}
