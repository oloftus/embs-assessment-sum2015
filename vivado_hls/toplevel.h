#include <hls_stream.h>

#include "types.h"

#ifndef __TOPLEVEL_H_
#define __TOPLEVEL_H_

void toplevel(hls::stream<uint32> &in, hls::stream<uint32> &out);

#endif
