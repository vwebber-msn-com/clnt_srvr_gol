#pragma once

#define SUPPORT_FOR_C_EXCEPTIONS FALSE
#define uint8  unsigned __int8
#define uint16 unsigned __int16
#define uint32 unsigned __int32
#define uint64 unsigned __int64

#define EXIT_ERROR 1
#define SLN_PACK    16

// RUNTIME PARAM
// Array sizes allowed on 16GB Windows 10
// max 10737418
// ok  10737000
// ok  11737000
// ok  30000000 pc ok
// ok  51737000
// ok  61737000
// ok  65000000 pc err
// err 70000000
// err 91737000
// err 150000000
// err 100000000


#define GM_INSTANCE_CNT 20

