#include <pnoise.h>
#include <math.h>
#include <stdint.h>

#include <string.h>
#include <stdlib.h>

#define FADE(t) ( (t) * (t) * (t) * ( (t) * ( (t) * 6 - 15 ) + 10 )) 
#define LERP(a,b,x) ( (a) + (x) * ( (b) - (a) ) )

uint8_t p[512] = { 180,160,137,91,90,15,                  
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,    
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,155,101,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,151,
    
    180,160,137,91,90,15,                  
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,    
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,155,101,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,151
};

float grad(int hash, float x, float y, float z)
{
    switch(hash & 0xF)
    {
        case 0x0: return  x + y;
        case 0x1: return -x + y;
        case 0x2: return  x - y;
        case 0x3: return -x - y;
        case 0x4: return  x + z;
        case 0x5: return -x + z;
        case 0x6: return  x - z;
        case 0x7: return -x - z;
        case 0x8: return  y + z;
        case 0x9: return -y + z;
        case 0xA: return  y - z;
        case 0xB: return -y - z;
        case 0xC: return  y + x;
        case 0xD: return -y + z;
        case 0xE: return  y - x;
        case 0xF: return -y - z;
        default: return 0; // never happens
    }
}

float noise (float x, float y, float z){
	
	// If input is negative offset more into the negative to avoid int rounding and make the "quadrants" join up
	x = (x <= 0) ? (x - 1.0f): x;
	y = (y <= 0) ? (y - 1.0f): y;
	z = (z <= 0) ? (z - 1.0f): z;
	
	int int_x = (int)x;
	int int_y = (int)y;
	int int_z = (int)z;
	int32_t xi = int_x & 255;
	int32_t yi = int_y & 255;
	int32_t zi = int_z & 255;
	float xf = (x-int_x);
	float yf = (y-int_y);
	float zf = (z-int_z);
	
	// If negative inputs, convert to values between 1 and  0 (e.g. -0.9 -> 0.1)
	xf = (x > 0) ? xf : (1 + xf); 
	yf = (y > 0) ? yf : (1 + yf);
	zf = (z > 0) ? zf : (1 + zf);
	
	// Interpolation values
	float u = FADE(xf);
	float v = FADE(yf);
	float w = FADE(zf);
	
	// Hashed Coordinates
	int aaa, aba, aab, abb, baa, bba, bab, bbb;
	aaa = p[p[p[    xi ]+    yi ]+    zi ];
	aba = p[p[p[    xi ]+yi + 1 ]+    zi ];
	aab = p[p[p[    xi ]+    yi ]+zi + 1 ];
	abb = p[p[p[    xi ]+yi + 1 ]+zi + 1 ];
	baa = p[p[p[xi + 1 ]+    yi ]+    zi ];
	bba = p[p[p[xi + 1 ]+yi + 1 ]+    zi ];
	bab = p[p[p[xi + 1 ]+    yi ]+zi + 1 ];
	bbb = p[p[p[xi + 1 ]+yi + 1 ]+zi + 1 ];
	
	float x1,x2,y1,y2;
	
	x1 = LERP(grad (aaa, xf  , yf  , zf), grad (baa, xf-1, yf  , zf), u);     
	
	x2 = LERP(grad (aba, xf  , yf-1, zf), grad (bba, xf-1, yf-1, zf), u);
	
	y1 = LERP(x1, x2, v);
	
	x1 = LERP(grad (aab, xf  , yf  , zf-1), grad (bab, xf-1, yf  , zf-1), u);
	
	x2 = LERP(grad (abb, xf  , yf-1, zf-1), grad (bbb, xf-1, yf-1, zf-1), u);
	
	y2 = LERP (x1, x2, v);
	
	return LERP(y1,y2,w);
}

void seeded_noise_shuffle (int seed){
	srand (seed);
	
	for (int i = 0; i < 1024; i++){
		int a = rand () % 256;
		int b = rand () % 256;
		
		uint8_t temp = p[a];
		p[a] = p[b];
		p[b] = temp;
	}
	
	memcpy(p + 256, p, 256);
	
}
