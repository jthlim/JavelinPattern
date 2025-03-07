//==========================================================================

#include "Javelin/Pattern/Internal/PatternTypes.h"

//==========================================================================

using namespace Javelin::PatternInternal;

//==========================================================================

constexpr CaseConversionData CaseConversionData::DATA[] =
{
	// CaseFold Table: 2474 pairs, 338 ranges
	{ { 65, 90 }, 32 },
	{ { 73, 73 }, 231 },
	{ { 73, 73 }, 232 },
	{ { 75, 75 }, 8415 },
	{ { 83, 83 }, 300 },
	{ { 97, 122 }, -32 },
	{ { 105, 105 }, 199 },
	{ { 105, 105 }, 200 },
	{ { 107, 107 }, 8383 },
	{ { 115, 115 }, 268 },
	{ { 181, 181 }, 743 },
	{ { 181, 181 }, 775 },
	{ { 192, 214 }, 32 },
	{ { 197, 197 }, 8294 },
	{ { 216, 222 }, 32 },
	{ { 223, 223 }, 7615 },
	{ { 224, 246 }, -32 },
	{ { 229, 229 }, 8262 },
	{ { 248, 254 }, -32 },
	{ { 255, 255 }, 121 },
	{ { 256, 311 }, MERGE_EVEN_ODD },
	{ { 304, 305 }, MERGE_FIXED | 73 },
	{ { 304, 305 }, MERGE_FIXED | 105 },
	{ { 313, 328 }, MERGE_ODD_EVEN },
	{ { 330, 375 }, MERGE_EVEN_ODD },
	{ { 376, 376 }, -121 },
	{ { 377, 382 }, MERGE_ODD_EVEN },
	{ { 383, 383 }, -300 },
	{ { 383, 383 }, -268 },
	{ { 384, 384 }, 195 },
	{ { 385, 385 }, 210 },
	{ { 386, 389 }, MERGE_EVEN_ODD },
	{ { 390, 390 }, 206 },
	{ { 391, 392 }, MERGE_ODD_EVEN },
	{ { 393, 394 }, 205 },
	{ { 395, 396 }, MERGE_ODD_EVEN },
	{ { 398, 398 }, 79 },
	{ { 399, 399 }, 202 },
	{ { 400, 400 }, 203 },
	{ { 401, 402 }, MERGE_ODD_EVEN },
	{ { 403, 403 }, 205 },
	{ { 404, 404 }, 207 },
	{ { 405, 405 }, 97 },
	{ { 406, 406 }, 211 },
	{ { 407, 407 }, 209 },
	{ { 408, 409 }, MERGE_EVEN_ODD },
	{ { 410, 410 }, 163 },
	{ { 412, 412 }, 211 },
	{ { 413, 413 }, 213 },
	{ { 414, 414 }, 130 },
	{ { 415, 415 }, 214 },
	{ { 416, 421 }, MERGE_EVEN_ODD },
	{ { 422, 422 }, 218 },
	{ { 423, 424 }, MERGE_ODD_EVEN },
	{ { 425, 425 }, 218 },
	{ { 428, 429 }, MERGE_EVEN_ODD },
	{ { 430, 430 }, 218 },
	{ { 431, 432 }, MERGE_ODD_EVEN },
	{ { 433, 434 }, 217 },
	{ { 435, 438 }, MERGE_ODD_EVEN },
	{ { 439, 439 }, 219 },
	{ { 440, 441 }, MERGE_EVEN_ODD },
	{ { 444, 445 }, MERGE_EVEN_ODD },
	{ { 447, 447 }, 56 },
	{ { 452, 454 }, MERGE_ALL },
	{ { 455, 457 }, MERGE_ALL },
	{ { 458, 460 }, MERGE_ALL },
	{ { 461, 476 }, MERGE_ODD_EVEN },
	{ { 477, 477 }, -79 },
	{ { 478, 495 }, MERGE_EVEN_ODD },
	{ { 497, 499 }, MERGE_ALL },
	{ { 500, 501 }, MERGE_EVEN_ODD },
	{ { 502, 502 }, -97 },
	{ { 503, 503 }, -56 },
	{ { 504, 543 }, MERGE_EVEN_ODD },
	{ { 544, 544 }, -130 },
	{ { 546, 563 }, MERGE_EVEN_ODD },
	{ { 570, 570 }, 10795 },
	{ { 571, 572 }, MERGE_ODD_EVEN },
	{ { 573, 573 }, -163 },
	{ { 574, 574 }, 10792 },
	{ { 575, 576 }, 10815 },
	{ { 577, 578 }, MERGE_ODD_EVEN },
	{ { 579, 579 }, -195 },
	{ { 580, 580 }, 69 },
	{ { 581, 581 }, 71 },
	{ { 582, 591 }, MERGE_EVEN_ODD },
	{ { 592, 592 }, 10783 },
	{ { 593, 593 }, 10780 },
	{ { 594, 594 }, 10782 },
	{ { 595, 595 }, -210 },
	{ { 596, 596 }, -206 },
	{ { 598, 599 }, -205 },
	{ { 601, 601 }, -202 },
	{ { 603, 603 }, -203 },
	{ { 604, 604 }, 42319 },
	{ { 608, 608 }, -205 },
	{ { 609, 609 }, 42315 },
	{ { 611, 611 }, -207 },
	{ { 613, 613 }, 42280 },
	{ { 614, 614 }, 42308 },
	{ { 616, 616 }, -209 },
	{ { 617, 617 }, -211 },
	{ { 619, 619 }, 10743 },
	{ { 620, 620 }, 42305 },
	{ { 623, 623 }, -211 },
	{ { 625, 625 }, 10749 },
	{ { 626, 626 }, -213 },
	{ { 629, 629 }, -214 },
	{ { 637, 637 }, 10727 },
	{ { 640, 640 }, -218 },
	{ { 643, 643 }, -218 },
	{ { 647, 647 }, 42282 },
	{ { 648, 648 }, -218 },
	{ { 649, 649 }, -69 },
	{ { 650, 651 }, -217 },
	{ { 652, 652 }, -71 },
	{ { 658, 658 }, -219 },
	{ { 669, 669 }, 42261 },
	{ { 670, 670 }, 42258 },
	{ { 837, 837 }, 84 },
	{ { 837, 837 }, 116 },
	{ { 837, 837 }, 7289 },
	{ { 880, 883 }, MERGE_EVEN_ODD },
	{ { 886, 887 }, MERGE_EVEN_ODD },
	{ { 891, 893 }, 130 },
	{ { 895, 895 }, 116 },
	{ { 902, 902 }, 38 },
	{ { 904, 906 }, 37 },
	{ { 908, 908 }, 64 },
	{ { 910, 911 }, 63 },
	{ { 913, 930 }, 32 },
	{ { 914, 914 }, 62 },
	{ { 917, 917 }, 96 },
	{ { 920, 920 }, 57 },
	{ { 920, 920 }, 92 },
	{ { 921, 921 }, -84 },
	{ { 921, 921 }, 7205 },
	{ { 922, 922 }, 86 },
	{ { 924, 924 }, -743 },
	{ { 928, 928 }, 54 },
	{ { 929, 929 }, 80 },
	{ { 931, 931 }, 32 },
	{ { 932, 939 }, 32 },
	{ { 934, 934 }, 47 },
	{ { 937, 937 }, 7549 },
	{ { 940, 940 }, -38 },
	{ { 941, 943 }, -37 },
	{ { 945, 961 }, -32 },
	{ { 946, 946 }, 30 },
	{ { 949, 949 }, 64 },
	{ { 952, 952 }, 25 },
	{ { 952, 952 }, 60 },
	{ { 953, 953 }, -116 },
	{ { 953, 953 }, 7173 },
	{ { 954, 954 }, 54 },
	{ { 956, 956 }, -775 },
	{ { 960, 960 }, 22 },
	{ { 961, 961 }, 48 },
	{ { 962, 963 }, MERGE_EVEN_ODD },
	{ { 962, 962 }, -31 },
	{ { 963, 971 }, -32 },
	{ { 966, 966 }, 15 },
	{ { 969, 969 }, 7517 },
	{ { 972, 972 }, -64 },
	{ { 973, 974 }, -63 },
	{ { 975, 975 }, 8 },
	{ { 976, 976 }, -62 },
	{ { 976, 976 }, -30 },
	{ { 977, 977 }, -57 },
	{ { 977, 977 }, -25 },
	{ { 977, 977 }, 35 },
	{ { 981, 981 }, -47 },
	{ { 981, 981 }, -15 },
	{ { 982, 982 }, -54 },
	{ { 982, 982 }, -22 },
	{ { 983, 983 }, -8 },
	{ { 984, 1007 }, MERGE_EVEN_ODD },
	{ { 1008, 1008 }, -86 },
	{ { 1008, 1008 }, -54 },
	{ { 1009, 1009 }, -80 },
	{ { 1009, 1009 }, -48 },
	{ { 1010, 1010 }, 7 },
	{ { 1011, 1011 }, -116 },
	{ { 1012, 1012 }, -92 },
	{ { 1012, 1012 }, -60 },
	{ { 1012, 1012 }, -35 },
	{ { 1013, 1013 }, -96 },
	{ { 1013, 1013 }, -64 },
	{ { 1015, 1016 }, MERGE_ODD_EVEN },
	{ { 1017, 1017 }, -7 },
	{ { 1018, 1019 }, MERGE_EVEN_ODD },
	{ { 1021, 1023 }, -130 },
	{ { 1024, 1039 }, 80 },
	{ { 1040, 1071 }, 32 },
	{ { 1072, 1103 }, -32 },
	{ { 1104, 1119 }, -80 },
	{ { 1120, 1153 }, MERGE_EVEN_ODD },
	{ { 1162, 1215 }, MERGE_EVEN_ODD },
	{ { 1216, 1216 }, 15 },
	{ { 1217, 1230 }, MERGE_ODD_EVEN },
	{ { 1231, 1231 }, -15 },
	{ { 1232, 1327 }, MERGE_EVEN_ODD },
	{ { 1329, 1366 }, 48 },
	{ { 1377, 1414 }, -48 },
	{ { 4256, 4293 }, 7264 },
	{ { 4295, 4295 }, 7264 },
	{ { 4301, 4301 }, 7264 },
	{ { 5024, 5103 }, 38864 },
	{ { 5104, 5109 }, 8 },
	{ { 5112, 5117 }, -8 },
	{ { 7545, 7545 }, 35332 },
	{ { 7549, 7549 }, 3814 },
	{ { 7680, 7829 }, MERGE_EVEN_ODD },
	{ { 7776, 7777 }, MERGE_FIXED | 7835 },
	{ { 7835, 7835 }, -59 },
	{ { 7835, 7835 }, -58 },
	{ { 7838, 7838 }, -7615 },
	{ { 7840, 7935 }, MERGE_EVEN_ODD },
	{ { 7936, 7943 }, 8 },
	{ { 7944, 7951 }, -8 },
	{ { 7952, 7957 }, 8 },
	{ { 7960, 7965 }, -8 },
	{ { 7968, 7975 }, 8 },
	{ { 7976, 7983 }, -8 },
	{ { 7984, 7991 }, 8 },
	{ { 7992, 7999 }, -8 },
	{ { 8000, 8005 }, 8 },
	{ { 8008, 8013 }, -8 },
	{ { 8017, 8017 }, 8 },
	{ { 8019, 8019 }, 8 },
	{ { 8021, 8021 }, 8 },
	{ { 8023, 8023 }, 8 },
	{ { 8025, 8025 }, -8 },
	{ { 8027, 8027 }, -8 },
	{ { 8029, 8029 }, -8 },
	{ { 8031, 8031 }, -8 },
	{ { 8032, 8039 }, 8 },
	{ { 8040, 8047 }, -8 },
	{ { 8048, 8049 }, 74 },
	{ { 8050, 8053 }, 86 },
	{ { 8054, 8055 }, 100 },
	{ { 8056, 8057 }, 128 },
	{ { 8058, 8059 }, 112 },
	{ { 8060, 8061 }, 126 },
	{ { 8064, 8071 }, 8 },
	{ { 8072, 8079 }, -8 },
	{ { 8080, 8087 }, 8 },
	{ { 8088, 8095 }, -8 },
	{ { 8096, 8103 }, 8 },
	{ { 8104, 8111 }, -8 },
	{ { 8112, 8113 }, 8 },
	{ { 8115, 8115 }, 9 },
	{ { 8120, 8121 }, -8 },
	{ { 8122, 8123 }, -74 },
	{ { 8124, 8124 }, -9 },
	{ { 8126, 8126 }, -7289 },
	{ { 8126, 8126 }, -7205 },
	{ { 8126, 8126 }, -7173 },
	{ { 8131, 8131 }, 9 },
	{ { 8136, 8139 }, -86 },
	{ { 8140, 8140 }, -9 },
	{ { 8144, 8145 }, 8 },
	{ { 8152, 8153 }, -8 },
	{ { 8154, 8155 }, -100 },
	{ { 8160, 8161 }, 8 },
	{ { 8165, 8165 }, 7 },
	{ { 8168, 8169 }, -8 },
	{ { 8170, 8171 }, -112 },
	{ { 8172, 8172 }, -7 },
	{ { 8179, 8179 }, 9 },
	{ { 8184, 8185 }, -128 },
	{ { 8186, 8187 }, -126 },
	{ { 8188, 8188 }, -9 },
	{ { 8486, 8486 }, -7549 },
	{ { 8486, 8486 }, -7517 },
	{ { 8490, 8490 }, -8415 },
	{ { 8490, 8490 }, -8383 },
	{ { 8491, 8491 }, -8294 },
	{ { 8491, 8491 }, -8262 },
	{ { 8498, 8498 }, 28 },
	{ { 8526, 8526 }, -28 },
	{ { 8544, 8559 }, 16 },
	{ { 8560, 8575 }, -16 },
	{ { 8579, 8580 }, MERGE_ODD_EVEN },
	{ { 9398, 9423 }, 26 },
	{ { 9424, 9449 }, -26 },
	{ { 11264, 11310 }, 48 },
	{ { 11312, 11358 }, -48 },
	{ { 11360, 11361 }, MERGE_EVEN_ODD },
	{ { 11362, 11362 }, -10743 },
	{ { 11363, 11363 }, -3814 },
	{ { 11364, 11364 }, -10727 },
	{ { 11365, 11365 }, -10795 },
	{ { 11366, 11366 }, -10792 },
	{ { 11367, 11372 }, MERGE_ODD_EVEN },
	{ { 11373, 11373 }, -10780 },
	{ { 11374, 11374 }, -10749 },
	{ { 11375, 11375 }, -10783 },
	{ { 11376, 11376 }, -10782 },
	{ { 11378, 11379 }, MERGE_EVEN_ODD },
	{ { 11381, 11382 }, MERGE_ODD_EVEN },
	{ { 11390, 11391 }, -10815 },
	{ { 11392, 11491 }, MERGE_EVEN_ODD },
	{ { 11499, 11502 }, MERGE_ODD_EVEN },
	{ { 11506, 11507 }, MERGE_EVEN_ODD },
	{ { 11520, 11557 }, -7264 },
	{ { 11559, 11559 }, -7264 },
	{ { 11565, 11565 }, -7264 },
	{ { 42560, 42605 }, MERGE_EVEN_ODD },
	{ { 42624, 42651 }, MERGE_EVEN_ODD },
	{ { 42786, 42799 }, MERGE_EVEN_ODD },
	{ { 42802, 42863 }, MERGE_EVEN_ODD },
	{ { 42873, 42876 }, MERGE_ODD_EVEN },
	{ { 42877, 42877 }, -35332 },
	{ { 42878, 42887 }, MERGE_EVEN_ODD },
	{ { 42891, 42892 }, MERGE_ODD_EVEN },
	{ { 42893, 42893 }, -42280 },
	{ { 42896, 42899 }, MERGE_EVEN_ODD },
	{ { 42902, 42921 }, MERGE_EVEN_ODD },
	{ { 42922, 42922 }, -42308 },
	{ { 42923, 42923 }, -42319 },
	{ { 42924, 42924 }, -42315 },
	{ { 42925, 42925 }, -42305 },
	{ { 42928, 42928 }, -42258 },
	{ { 42929, 42929 }, -42282 },
	{ { 42930, 42930 }, -42261 },
	{ { 42931, 42931 }, 928 },
	{ { 42932, 42935 }, MERGE_EVEN_ODD },
	{ { 43888, 43967 }, -38864 },
	{ { 65313, 65338 }, 32 },
	{ { 65345, 65370 }, -32 },
	{ { 66560, 66599 }, 40 },
	{ { 66600, 66639 }, -40 },
	{ { 68736, 68786 }, 64 },
	{ { 68800, 68850 }, -64 },
	{ { 71840, 71871 }, 32 },
	{ { 71872, 71903 }, -32 },
};

const uint32_t CaseConversionData::COUNT = 338;

//==========================================================================

