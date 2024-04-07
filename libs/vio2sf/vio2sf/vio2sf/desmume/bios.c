/*  Copyright (C) 2006 yopyop
    yopyop156@ifrance.com
    yopyop156.ifrance.com

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "cp15.h"
#include <math.h>
#include "MMU.h"
#include "spu_exports.h"
#include "debug.h"
#include "isqrt.h"

#include "state.h"

static const u16 getsinetbl[] = {
0x0000, 0x0324, 0x0648, 0x096A, 0x0C8C, 0x0FAB, 0x12C8, 0x15E2, 
0x18F9, 0x1C0B, 0x1F1A, 0x2223, 0x2528, 0x2826, 0x2B1F, 0x2E11, 
0x30FB, 0x33DF, 0x36BA, 0x398C, 0x3C56, 0x3F17, 0x41CE, 0x447A, 
0x471C, 0x49B4, 0x4C3F, 0x4EBF, 0x5133, 0x539B, 0x55F5, 0x5842, 
0x5A82, 0x5CB3, 0x5ED7, 0x60EB, 0x62F1, 0x64E8, 0x66CF, 0x68A6, 
0x6A6D, 0x6C23, 0x6DC9, 0x6F5E, 0x70E2, 0x7254, 0x73B5, 0x7504, 
0x7641, 0x776B, 0x7884, 0x7989, 0x7A7C, 0x7B5C, 0x7C29, 0x7CE3, 
0x7D89, 0x7E1D, 0x7E9C, 0x7F09, 0x7F61, 0x7FA6, 0x7FD8, 0x7FF5
};

static const u16 getpitchtbl[] = {
0x0000, 0x003B, 0x0076, 0x00B2, 0x00ED, 0x0128, 0x0164, 0x019F, 
0x01DB, 0x0217, 0x0252, 0x028E, 0x02CA, 0x0305, 0x0341, 0x037D, 
0x03B9, 0x03F5, 0x0431, 0x046E, 0x04AA, 0x04E6, 0x0522, 0x055F, 
0x059B, 0x05D8, 0x0614, 0x0651, 0x068D, 0x06CA, 0x0707, 0x0743, 
0x0780, 0x07BD, 0x07FA, 0x0837, 0x0874, 0x08B1, 0x08EF, 0x092C, 
0x0969, 0x09A7, 0x09E4, 0x0A21, 0x0A5F, 0x0A9C, 0x0ADA, 0x0B18, 
0x0B56, 0x0B93, 0x0BD1, 0x0C0F, 0x0C4D, 0x0C8B, 0x0CC9, 0x0D07, 
0x0D45, 0x0D84, 0x0DC2, 0x0E00, 0x0E3F, 0x0E7D, 0x0EBC, 0x0EFA, 
0x0F39, 0x0F78, 0x0FB6, 0x0FF5, 0x1034, 0x1073, 0x10B2, 0x10F1, 
0x1130, 0x116F, 0x11AE, 0x11EE, 0x122D, 0x126C, 0x12AC, 0x12EB, 
0x132B, 0x136B, 0x13AA, 0x13EA, 0x142A, 0x146A, 0x14A9, 0x14E9, 
0x1529, 0x1569, 0x15AA, 0x15EA, 0x162A, 0x166A, 0x16AB, 0x16EB, 
0x172C, 0x176C, 0x17AD, 0x17ED, 0x182E, 0x186F, 0x18B0, 0x18F0, 
0x1931, 0x1972, 0x19B3, 0x19F5, 0x1A36, 0x1A77, 0x1AB8, 0x1AFA, 
0x1B3B, 0x1B7D, 0x1BBE, 0x1C00, 0x1C41, 0x1C83, 0x1CC5, 0x1D07, 
0x1D48, 0x1D8A, 0x1DCC, 0x1E0E, 0x1E51, 0x1E93, 0x1ED5, 0x1F17, 
0x1F5A, 0x1F9C, 0x1FDF, 0x2021, 0x2064, 0x20A6, 0x20E9, 0x212C, 
0x216F, 0x21B2, 0x21F5, 0x2238, 0x227B, 0x22BE, 0x2301, 0x2344, 
0x2388, 0x23CB, 0x240E, 0x2452, 0x2496, 0x24D9, 0x251D, 0x2561, 
0x25A4, 0x25E8, 0x262C, 0x2670, 0x26B4, 0x26F8, 0x273D, 0x2781, 
0x27C5, 0x280A, 0x284E, 0x2892, 0x28D7, 0x291C, 0x2960, 0x29A5, 
0x29EA, 0x2A2F, 0x2A74, 0x2AB9, 0x2AFE, 0x2B43, 0x2B88, 0x2BCD, 
0x2C13, 0x2C58, 0x2C9D, 0x2CE3, 0x2D28, 0x2D6E, 0x2DB4, 0x2DF9, 
0x2E3F, 0x2E85, 0x2ECB, 0x2F11, 0x2F57, 0x2F9D, 0x2FE3, 0x302A, 
0x3070, 0x30B6, 0x30FD, 0x3143, 0x318A, 0x31D0, 0x3217, 0x325E, 
0x32A5, 0x32EC, 0x3332, 0x3379, 0x33C1, 0x3408, 0x344F, 0x3496, 
0x34DD, 0x3525, 0x356C, 0x35B4, 0x35FB, 0x3643, 0x368B, 0x36D3, 
0x371A, 0x3762, 0x37AA, 0x37F2, 0x383A, 0x3883, 0x38CB, 0x3913, 
0x395C, 0x39A4, 0x39ED, 0x3A35, 0x3A7E, 0x3AC6, 0x3B0F, 0x3B58, 
0x3BA1, 0x3BEA, 0x3C33, 0x3C7C, 0x3CC5, 0x3D0E, 0x3D58, 0x3DA1, 
0x3DEA, 0x3E34, 0x3E7D, 0x3EC7, 0x3F11, 0x3F5A, 0x3FA4, 0x3FEE, 
0x4038, 0x4082, 0x40CC, 0x4116, 0x4161, 0x41AB, 0x41F5, 0x4240, 
0x428A, 0x42D5, 0x431F, 0x436A, 0x43B5, 0x4400, 0x444B, 0x4495, 
0x44E1, 0x452C, 0x4577, 0x45C2, 0x460D, 0x4659, 0x46A4, 0x46F0, 
0x473B, 0x4787, 0x47D3, 0x481E, 0x486A, 0x48B6, 0x4902, 0x494E, 
0x499A, 0x49E6, 0x4A33, 0x4A7F, 0x4ACB, 0x4B18, 0x4B64, 0x4BB1, 
0x4BFE, 0x4C4A, 0x4C97, 0x4CE4, 0x4D31, 0x4D7E, 0x4DCB, 0x4E18, 
0x4E66, 0x4EB3, 0x4F00, 0x4F4E, 0x4F9B, 0x4FE9, 0x5036, 0x5084, 
0x50D2, 0x5120, 0x516E, 0x51BC, 0x520A, 0x5258, 0x52A6, 0x52F4, 
0x5343, 0x5391, 0x53E0, 0x542E, 0x547D, 0x54CC, 0x551A, 0x5569, 
0x55B8, 0x5607, 0x5656, 0x56A5, 0x56F4, 0x5744, 0x5793, 0x57E2, 
0x5832, 0x5882, 0x58D1, 0x5921, 0x5971, 0x59C1, 0x5A10, 0x5A60, 
0x5AB0, 0x5B01, 0x5B51, 0x5BA1, 0x5BF1, 0x5C42, 0x5C92, 0x5CE3, 
0x5D34, 0x5D84, 0x5DD5, 0x5E26, 0x5E77, 0x5EC8, 0x5F19, 0x5F6A, 
0x5FBB, 0x600D, 0x605E, 0x60B0, 0x6101, 0x6153, 0x61A4, 0x61F6, 
0x6248, 0x629A, 0x62EC, 0x633E, 0x6390, 0x63E2, 0x6434, 0x6487, 
0x64D9, 0x652C, 0x657E, 0x65D1, 0x6624, 0x6676, 0x66C9, 0x671C, 
0x676F, 0x67C2, 0x6815, 0x6869, 0x68BC, 0x690F, 0x6963, 0x69B6, 
0x6A0A, 0x6A5E, 0x6AB1, 0x6B05, 0x6B59, 0x6BAD, 0x6C01, 0x6C55, 
0x6CAA, 0x6CFE, 0x6D52, 0x6DA7, 0x6DFB, 0x6E50, 0x6EA4, 0x6EF9, 
0x6F4E, 0x6FA3, 0x6FF8, 0x704D, 0x70A2, 0x70F7, 0x714D, 0x71A2, 
0x71F7, 0x724D, 0x72A2, 0x72F8, 0x734E, 0x73A4, 0x73FA, 0x7450, 
0x74A6, 0x74FC, 0x7552, 0x75A8, 0x75FF, 0x7655, 0x76AC, 0x7702, 
0x7759, 0x77B0, 0x7807, 0x785E, 0x78B4, 0x790C, 0x7963, 0x79BA, 
0x7A11, 0x7A69, 0x7AC0, 0x7B18, 0x7B6F, 0x7BC7, 0x7C1F, 0x7C77, 
0x7CCF, 0x7D27, 0x7D7F, 0x7DD7, 0x7E2F, 0x7E88, 0x7EE0, 0x7F38, 
0x7F91, 0x7FEA, 0x8042, 0x809B, 0x80F4, 0x814D, 0x81A6, 0x81FF, 
0x8259, 0x82B2, 0x830B, 0x8365, 0x83BE, 0x8418, 0x8472, 0x84CB, 
0x8525, 0x857F, 0x85D9, 0x8633, 0x868E, 0x86E8, 0x8742, 0x879D, 
0x87F7, 0x8852, 0x88AC, 0x8907, 0x8962, 0x89BD, 0x8A18, 0x8A73, 
0x8ACE, 0x8B2A, 0x8B85, 0x8BE0, 0x8C3C, 0x8C97, 0x8CF3, 0x8D4F, 
0x8DAB, 0x8E07, 0x8E63, 0x8EBF, 0x8F1B, 0x8F77, 0x8FD4, 0x9030, 
0x908C, 0x90E9, 0x9146, 0x91A2, 0x91FF, 0x925C, 0x92B9, 0x9316, 
0x9373, 0x93D1, 0x942E, 0x948C, 0x94E9, 0x9547, 0x95A4, 0x9602, 
0x9660, 0x96BE, 0x971C, 0x977A, 0x97D8, 0x9836, 0x9895, 0x98F3, 
0x9952, 0x99B0, 0x9A0F, 0x9A6E, 0x9ACD, 0x9B2C, 0x9B8B, 0x9BEA, 
0x9C49, 0x9CA8, 0x9D08, 0x9D67, 0x9DC7, 0x9E26, 0x9E86, 0x9EE6, 
0x9F46, 0x9FA6, 0xA006, 0xA066, 0xA0C6, 0xA127, 0xA187, 0xA1E8, 
0xA248, 0xA2A9, 0xA30A, 0xA36B, 0xA3CC, 0xA42D, 0xA48E, 0xA4EF, 
0xA550, 0xA5B2, 0xA613, 0xA675, 0xA6D6, 0xA738, 0xA79A, 0xA7FC, 
0xA85E, 0xA8C0, 0xA922, 0xA984, 0xA9E7, 0xAA49, 0xAAAC, 0xAB0E, 
0xAB71, 0xABD4, 0xAC37, 0xAC9A, 0xACFD, 0xAD60, 0xADC3, 0xAE27, 
0xAE8A, 0xAEED, 0xAF51, 0xAFB5, 0xB019, 0xB07C, 0xB0E0, 0xB145, 
0xB1A9, 0xB20D, 0xB271, 0xB2D6, 0xB33A, 0xB39F, 0xB403, 0xB468, 
0xB4CD, 0xB532, 0xB597, 0xB5FC, 0xB662, 0xB6C7, 0xB72C, 0xB792, 
0xB7F7, 0xB85D, 0xB8C3, 0xB929, 0xB98F, 0xB9F5, 0xBA5B, 0xBAC1, 
0xBB28, 0xBB8E, 0xBBF5, 0xBC5B, 0xBCC2, 0xBD29, 0xBD90, 0xBDF7, 
0xBE5E, 0xBEC5, 0xBF2C, 0xBF94, 0xBFFB, 0xC063, 0xC0CA, 0xC132, 
0xC19A, 0xC202, 0xC26A, 0xC2D2, 0xC33A, 0xC3A2, 0xC40B, 0xC473, 
0xC4DC, 0xC544, 0xC5AD, 0xC616, 0xC67F, 0xC6E8, 0xC751, 0xC7BB, 
0xC824, 0xC88D, 0xC8F7, 0xC960, 0xC9CA, 0xCA34, 0xCA9E, 0xCB08, 
0xCB72, 0xCBDC, 0xCC47, 0xCCB1, 0xCD1B, 0xCD86, 0xCDF1, 0xCE5B, 
0xCEC6, 0xCF31, 0xCF9C, 0xD008, 0xD073, 0xD0DE, 0xD14A, 0xD1B5, 
0xD221, 0xD28D, 0xD2F8, 0xD364, 0xD3D0, 0xD43D, 0xD4A9, 0xD515, 
0xD582, 0xD5EE, 0xD65B, 0xD6C7, 0xD734, 0xD7A1, 0xD80E, 0xD87B, 
0xD8E9, 0xD956, 0xD9C3, 0xDA31, 0xDA9E, 0xDB0C, 0xDB7A, 0xDBE8, 
0xDC56, 0xDCC4, 0xDD32, 0xDDA0, 0xDE0F, 0xDE7D, 0xDEEC, 0xDF5B, 
0xDFC9, 0xE038, 0xE0A7, 0xE116, 0xE186, 0xE1F5, 0xE264, 0xE2D4, 
0xE343, 0xE3B3, 0xE423, 0xE493, 0xE503, 0xE573, 0xE5E3, 0xE654, 
0xE6C4, 0xE735, 0xE7A5, 0xE816, 0xE887, 0xE8F8, 0xE969, 0xE9DA, 
0xEA4B, 0xEABC, 0xEB2E, 0xEB9F, 0xEC11, 0xEC83, 0xECF5, 0xED66, 
0xEDD9, 0xEE4B, 0xEEBD, 0xEF2F, 0xEFA2, 0xF014, 0xF087, 0xF0FA, 
0xF16D, 0xF1E0, 0xF253, 0xF2C6, 0xF339, 0xF3AD, 0xF420, 0xF494, 
0xF507, 0xF57B, 0xF5EF, 0xF663, 0xF6D7, 0xF74C, 0xF7C0, 0xF834, 
0xF8A9, 0xF91E, 0xF992, 0xFA07, 0xFA7C, 0xFAF1, 0xFB66, 0xFBDC, 
0xFC51, 0xFCC7, 0xFD3C, 0xFDB2, 0xFE28, 0xFE9E, 0xFF14, 0xFF8A
};

static const u8 getvoltbl[] = {
0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 
0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 
0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 
0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 
0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 
0x09, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 
0x0B, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 
0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 
0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13, 0x14, 
0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x18, 
0x18, 0x18, 0x18, 0x19, 0x19, 0x19, 0x19, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B, 0x1B, 0x1C, 0x1C, 0x1C, 
0x1D, 0x1D, 0x1D, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F, 0x1F, 0x20, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 
0x22, 0x23, 0x23, 0x24, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27, 0x27, 0x28, 0x28, 0x29, 
0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F, 0x30, 0x31, 0x31, 
0x32, 0x32, 0x33, 0x33, 0x34, 0x35, 0x35, 0x36, 0x36, 0x37, 0x38, 0x38, 0x39, 0x3A, 0x3A, 0x3B, 
0x3C, 0x3C, 0x3D, 0x3E, 0x3F, 0x3F, 0x40, 0x41, 0x42, 0x42, 0x43, 0x44, 0x45, 0x45, 0x46, 0x47, 
0x48, 0x49, 0x4A, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x52, 0x53, 0x54, 0x55, 
0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x67, 
0x68, 0x69, 0x6A, 0x6B, 0x6D, 0x6E, 0x6F, 0x71, 0x72, 0x73, 0x75, 0x76, 0x77, 0x79, 0x7A, 0x7B, 
0x7D, 0x7E, 0x7F, 0x20, 0x21, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 
0x26, 0x26, 0x26, 0x27, 0x27, 0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2C, 0x2C, 0x2D, 
0x2D, 0x2E, 0x2E, 0x2F, 0x2F, 0x30, 0x30, 0x31, 0x31, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x36, 
0x36, 0x37, 0x37, 0x38, 0x39, 0x39, 0x3A, 0x3B, 0x3B, 0x3C, 0x3D, 0x3E, 0x3E, 0x3F, 0x40, 0x40, 
0x41, 0x42, 0x43, 0x43, 0x44, 0x45, 0x46, 0x47, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4D, 
0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 
0x5E, 0x5F, 0x60, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6F, 0x70, 
0x71, 0x73, 0x74, 0x75, 0x77, 0x78, 0x79, 0x7B, 0x7C, 0x7E, 0x7E, 0x40, 0x41, 0x42, 0x43, 0x43, 
0x44, 0x45, 0x46, 0x47, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 
0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 
0x62, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6B, 0x6C, 0x6D, 0x6E, 0x70, 0x71, 0x72, 0x74, 0x75, 
0x76, 0x78, 0x79, 0x7B, 0x7C, 0x7D, 0x7E, 0x40, 0x41, 0x42, 0x42, 0x43, 0x44, 0x45, 0x46, 0x46, 
0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 
0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x65, 0x66, 
0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6D, 0x6E, 0x6F, 0x71, 0x72, 0x73, 0x75, 0x76, 0x77, 0x79, 0x7A, 
0x7C, 0x7D, 0x7E, 0x7F
};

u32 bios_nop(armcpu_t * cpu)
{
     if (cpu->proc_ID == ARMCPU_ARM9)
     {
        LOG("Unimplemented bios function %02X(ARM9) was used. R0:%08X\n", (cpu->instruction)&0x1F, cpu->R[0]);
     }
     else
     {
        LOG("Unimplemented bios function %02X(ARM7) was used. R0:%08X\n", (cpu->instruction)&0x1F, cpu->R[0]);
     }
     return 3;
}

u32 delayLoop(armcpu_t * cpu)
{
     return cpu->R[0] * 4;
}

//u32 oldmode[2];

u32 intrWaitARM(armcpu_t * cpu)
{
     u32 intrFlagAdr;// = (((armcp15_t *)(cpu->coproc[15]))->DTCMRegion&0xFFFFF000)+0x3FF8;
     u32 intr;
     u32 intrFlag = 0;
     
     //cpu->state->execute = FALSE;
     if(cpu->proc_ID) 
     {
      intrFlagAdr = 0x380FFF8;
     } else {
      intrFlagAdr = (((armcp15_t *)(cpu->coproc[15]))->DTCMRegion&0xFFFFF000)+0x3FF8;
     }
     intr = MMU_read32(cpu->state, cpu->proc_ID, intrFlagAdr);
     intrFlag = cpu->R[1] & intr;
     
     if(intrFlag)
     {
          // si une(ou plusieurs) des interruptions que l'on attend s'est(se sont) produite(s)
          // on efface son(les) occurence(s).
          intr ^= intrFlag;
          MMU_write32(cpu->state, cpu->proc_ID, intrFlagAdr, intr);
          //cpu->switchMode(oldmode[cpu->proc_ID]);
          return 1;
     }
         
     cpu->R[15] = cpu->instruct_adr;
     cpu->next_instruction = cpu->R[15];
     cpu->waitIRQ = 1;
     //oldmode[cpu->proc_ID] = cpu->switchMode(SVC);

     return 1;
}

u32 waitVBlankARM(armcpu_t * cpu)
{
     u32 intrFlagAdr;// = (((armcp15_t *)(cpu->coproc[15]))->DTCMRegion&0xFFFFF000)+0x3FF8;
     u32 intr;
     u32 intrFlag = 0;
     
     //cpu->state->execute = FALSE;
     if(cpu->proc_ID) 
     {
      intrFlagAdr = 0x380FFF8;
     } else {
      intrFlagAdr = (((armcp15_t *)(cpu->coproc[15]))->DTCMRegion&0xFFFFF000)+0x3FF8;
     }
     intr = MMU_read32(cpu->state, cpu->proc_ID, intrFlagAdr);
     intrFlag = 1 & intr;
     
     if(intrFlag)
     {
          // si une(ou plusieurs) des interruptions que l'on attend s'est(se sont) produite(s)
          // on efface son(les) occurence(s).
          intr ^= intrFlag;
          MMU_write32(cpu->state, cpu->proc_ID, intrFlagAdr, intr);
          //cpu->switchMode(oldmode[cpu->proc_ID]);
          return 1;
     }
         
     cpu->R[15] = cpu->instruct_adr;
     cpu->next_instruction = cpu->R[15];
     cpu->waitIRQ = 1;
     //oldmode[cpu->proc_ID] = cpu->switchMode(SVC);

     return 1;
}

u32 wait4IRQ(armcpu_t* cpu)
{
     //cpu->state->execute= FALSE;
     if(cpu->wirq)
     {
          if(!cpu->waitIRQ)
          {
               cpu->waitIRQ = 0;
               cpu->wirq = 0;
               //cpu->switchMode(oldmode[cpu->proc_ID]);
               return 1;
          }
          cpu->R[15] = cpu->instruct_adr;
          cpu->next_instruction = cpu->R[15];
          return 1;
     }
     cpu->waitIRQ = 1;
     cpu->wirq = 1;
     cpu->R[15] = cpu->instruct_adr;
     cpu->next_instruction = cpu->R[15];
     //oldmode[cpu->proc_ID] = cpu->switchMode(SVC);
     return 1;
}

u32 devide(armcpu_t* cpu)
{
     s32 num = (s32)cpu->R[0];
     s32 dnum = (s32)cpu->R[1];
     
     if(dnum==0) return 0;
     
     cpu->R[0] = (u32)(num / dnum);
     cpu->R[1] = (u32)(num % dnum);
     cpu->R[3] = (u32) (((s32)cpu->R[0])<0 ? -((s32)cpu->R[0]) : cpu->R[0]);
     
     return 6;
}

u32 copy(armcpu_t* cpu)
{
     u32 src = cpu->R[0];
     u32 dst = cpu->R[1];
     u32 cnt = cpu->R[2];

     switch(BIT26(cnt))
     {
          case 0:
               src &= 0xFFFFFFFE;
               dst &= 0xFFFFFFFE;
               switch(BIT24(cnt))
               {
                    case 0:
                         cnt &= 0x1FFFFF;
                         while(cnt)
                         {
                              MMU_write16(cpu->state, cpu->proc_ID, dst, MMU_read16(cpu->state, cpu->proc_ID, src));
                              cnt--;
                              dst+=2;
                              src+=2;
                         }
                         break;
                    case 1:
                         {
                              u32 val = MMU_read16(cpu->state, cpu->proc_ID, src);
                              cnt &= 0x1FFFFF;
                              while(cnt)
                              {
                                   MMU_write16(cpu->state, cpu->proc_ID, dst, val);
                                   cnt--;
                                   dst+=2;
                              }
                         }
                         break;
               }
               break;
          case 1:
               src &= 0xFFFFFFFC;
               dst &= 0xFFFFFFFC;
               switch(BIT24(cnt))
               {
                    case 0:
                         cnt &= 0x1FFFFF;
                         while(cnt)
                         {
                              MMU_write32(cpu->state, cpu->proc_ID, dst, MMU_read32(cpu->state, cpu->proc_ID, src));
                              cnt--;
                              dst+=4;
                              src+=4;
                         }
                         break;
                    case 1:
                         {
                              u32 val = MMU_read32(cpu->state, cpu->proc_ID, src);
                              cnt &= 0x1FFFFF;
                              while(cnt)
                              {
                                   MMU_write32(cpu->state, cpu->proc_ID, dst, val);
                                   cnt--;
                                   dst+=4;
                              }
                         }
                         break;
               }
               break;
     }
     return 1;
}

u32 fastCopy(armcpu_t* cpu)
{
     u32 src = cpu->R[0] & 0xFFFFFFFC;
     u32 dst = cpu->R[1] & 0xFFFFFFFC;
     u32 cnt = cpu->R[2];

     switch(BIT24(cnt))
     {
          case 0:
               cnt &= 0x1FFFFF;
               while(cnt)
               {
                    MMU_write32(cpu->state, cpu->proc_ID, dst, MMU_read32(cpu->state, cpu->proc_ID, src));
                    cnt--;
                    dst+=4;
                    src+=4;
               }
               break;
          case 1:
               {
                    u32 val = MMU_read32(cpu->state, cpu->proc_ID, src);
                    cnt &= 0x1FFFFF;
                    while(cnt)
                    {
                         MMU_write32(cpu->state, cpu->proc_ID, dst, val);
                         cnt--;
                         dst+=4;
                    }
               }
               break;
     }
     return 1;
}

u32 LZ77UnCompVram(armcpu_t* cpu)
{
  int i1, i2;
  int byteCount;
  int byteShift;
  u32 writeValue;
  int len;
  u32 source = cpu->R[0];
  u32 dest = cpu->R[1];
  u32 header = MMU_read32(cpu->state, cpu->proc_ID, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return 0;    

  byteCount = 0;
  byteShift = 0;
  writeValue = 0;

  len = header >> 8;

  while(len > 0) {
    u8 d = MMU_read8(cpu->state, cpu->proc_ID, source++);

    if(d) {
      for(i1 = 0; i1 < 8; i1++) {
        if(d & 0x80) {
          int length;
          int offset;
          u32 windowOffset;
          u16 data = MMU_read8(cpu->state, cpu->proc_ID, source++) << 8;
          data |= MMU_read8(cpu->state, cpu->proc_ID, source++);
          length = (data >> 12) + 3;
          offset = (data & 0x0FFF);
          windowOffset = dest + byteCount - offset - 1;
          for(i2 = 0; i2 < length; i2++) {
            writeValue |= (MMU_read8(cpu->state, cpu->proc_ID, windowOffset++) << byteShift);
            byteShift += 8;
            byteCount++;

            if(byteCount == 2) {
              MMU_write16(cpu->state, cpu->proc_ID, dest, writeValue);
              dest += 2;
              byteCount = 0;
              byteShift = 0;
              writeValue = 0;
            }
            len--;
            if(len == 0)
              return 0;
          }
        } else {
          writeValue |= (MMU_read8(cpu->state, cpu->proc_ID, source++) << byteShift);
          byteShift += 8;
          byteCount++;
          if(byteCount == 2) {
            MMU_write16(cpu->state, cpu->proc_ID, dest, writeValue);
            dest += 2;
            byteCount = 0;
            byteShift = 0;
            writeValue = 0;
          }
          len--;
          if(len == 0)
            return 0;
        }
        d <<= 1;
      }
    } else {
      for(i1 = 0; i1 < 8; i1++) {
        writeValue |= (MMU_read8(cpu->state, cpu->proc_ID, source++) << byteShift);
        byteShift += 8;
        byteCount++;
        if(byteCount == 2) {
          MMU_write16(cpu->state, cpu->proc_ID, dest, writeValue);
          dest += 2;      
          byteShift = 0;
          byteCount = 0;
          writeValue = 0;
        }
        len--;
        if(len == 0)
          return 0;
      }
    }
  }
  return 1;
}

u32 LZ77UnCompWram(armcpu_t* cpu)
{
  int i1, i2;
  int len;
  u32 source = cpu->R[0];
  u32 dest = cpu->R[1];

  u32 header = MMU_read32(cpu->state, cpu->proc_ID, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return 0;  
  
  len = header >> 8;

  while(len > 0) {
    u8 d = MMU_read8(cpu->state, cpu->proc_ID, source++);

    if(d) {
      for(i1 = 0; i1 < 8; i1++) {
        if(d & 0x80) {
          int length;
          int offset;
          u32 windowOffset;
          u16 data = MMU_read8(cpu->state, cpu->proc_ID, source++) << 8;
          data |= MMU_read8(cpu->state, cpu->proc_ID, source++);
          length = (data >> 12) + 3;
          offset = (data & 0x0FFF);
          windowOffset = dest - offset - 1;
          for(i2 = 0; i2 < length; i2++) {
            MMU_write8(cpu->state, cpu->proc_ID, dest++, MMU_read8(cpu->state, cpu->proc_ID, windowOffset++));
            len--;
            if(len == 0)
              return 0;
          }
        } else {
          MMU_write8(cpu->state, cpu->proc_ID, dest++, MMU_read8(cpu->state, cpu->proc_ID, source++));
          len--;
          if(len == 0)
            return 0;
        }
        d <<= 1;
      }
    } else {
      for(i1 = 0; i1 < 8; i1++) {
        MMU_write8(cpu->state, cpu->proc_ID, dest++, MMU_read8(cpu->state, cpu->proc_ID, source++));
        len--;
        if(len == 0)
          return 0;
      }
    }
  }
  return 1;
}

u32 RLUnCompVram(armcpu_t* cpu)
{
  int i;
  int len;
  int byteCount;
  int byteShift;
  u32 writeValue;
  u32 source = cpu->R[0];
  u32 dest = cpu->R[1];

  u32 header = MMU_read32(cpu->state, cpu->proc_ID, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return 0;  
  
  len = header >> 8;
  byteCount = 0;
  byteShift = 0;
  writeValue = 0;

  while(len > 0) {
    u8 d = MMU_read8(cpu->state, cpu->proc_ID, source++);
    int l = d & 0x7F;
    if(d & 0x80) {
      u8 data = MMU_read8(cpu->state, cpu->proc_ID, source++);
      l += 3;
      for(i = 0;i < l; i++) {
        writeValue |= (data << byteShift);
        byteShift += 8;
        byteCount++;

        if(byteCount == 2) {
          MMU_write16(cpu->state, cpu->proc_ID, dest, writeValue);
          dest += 2;
          byteCount = 0;
          byteShift = 0;
          writeValue = 0;
        }
        len--;
        if(len == 0)
          return 0;
      }
    } else {
      l++;
      for(i = 0; i < l; i++) {
        writeValue |= (MMU_read8(cpu->state, cpu->proc_ID, source++) << byteShift);
        byteShift += 8;
        byteCount++;
        if(byteCount == 2) {
          MMU_write16(cpu->state, cpu->proc_ID, dest, writeValue);
          dest += 2;
          byteCount = 0;
          byteShift = 0;
          writeValue = 0;
        }
        len--;
        if(len == 0)
          return 0;
      }
    }
  }
  return 1;
}

u32 RLUnCompWram(armcpu_t* cpu)
{
  int i;
  int len;
  u32 source = cpu->R[0];
  u32 dest = cpu->R[1];

  u32 header = MMU_read32(cpu->state, cpu->proc_ID, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return 0;  
  
  len = header >> 8;

  while(len > 0) {
    u8 d = MMU_read8(cpu->state, cpu->proc_ID, source++);
    int l = d & 0x7F;
    if(d & 0x80) {
      u8 data = MMU_read8(cpu->state, cpu->proc_ID, source++);
      l += 3;
      for(i = 0;i < l; i++) {
        MMU_write8(cpu->state, cpu->proc_ID, dest++, data);
        len--;
        if(len == 0)
          return 0;
      }
    } else {
      l++;
      for(i = 0; i < l; i++) {
        MMU_write8(cpu->state, cpu->proc_ID, dest++,  MMU_read8(cpu->state, cpu->proc_ID, source++));
        len--;
        if(len == 0)
          return 0;
      }
    }
  }
  return 1;
}

u32 UnCompHuffman(armcpu_t* cpu)
{
  u32 source, dest, writeValue, header, treeStart, mask;
  u32 data;
  u8 treeSize, currentNode, rootNode;
  int byteCount, byteShift, len, pos;
  int writeData;

  source = cpu->R[0];
  dest = cpu->R[1];

  header = MMU_read8(cpu->state, cpu->proc_ID, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return 0;  
  
  treeSize = MMU_read8(cpu->state, cpu->proc_ID, source++);

  treeStart = source;

  source += ((treeSize+1)<<1)-1; // minus because we already skipped one byte
  
  len = header >> 8;

  mask = 0x80000000;
  data = MMU_read8(cpu->state, cpu->proc_ID, source);
  source += 4;

  pos = 0;
  rootNode = MMU_read8(cpu->state, cpu->proc_ID, treeStart);
  currentNode = rootNode;
  writeData = 0;
  byteShift = 0;
  byteCount = 0;
  writeValue = 0;

  if((header & 0x0F) == 8) {
    while(len > 0) {
      // take left
      if(pos == 0)
        pos++;
      else
        pos += (((currentNode & 0x3F)+1)<<1);
      
      if(data & mask) {
        // right
        if(currentNode & 0x40)
          writeData = 1;
        currentNode = MMU_read8(cpu->state, cpu->proc_ID, treeStart+pos+1);
      } else {
        // left
        if(currentNode & 0x80)
          writeData = 1;
        currentNode = MMU_read8(cpu->state, cpu->proc_ID, treeStart+pos);
      }
      
      if(writeData) {
        writeValue |= (currentNode << byteShift);
        byteCount++;
        byteShift += 8;

        pos = 0;
        currentNode = rootNode;
        writeData = 0;

        if(byteCount == 4) {
          byteCount = 0;
          byteShift = 0;
          MMU_write8(cpu->state, cpu->proc_ID, dest, writeValue);
          writeValue = 0;
          dest += 4;
          len -= 4;
        }
      }
      mask >>= 1;
      if(mask == 0) {
        mask = 0x80000000;
        data = MMU_read8(cpu->state, cpu->proc_ID, source);
        source += 4;
      }
    }
  } else {
    int halfLen = 0;
    int value = 0;
    while(len > 0) {
      // take left
      if(pos == 0)
        pos++;
      else
        pos += (((currentNode & 0x3F)+1)<<1);

      if((data & mask)) {
        // right
        if(currentNode & 0x40)
          writeData = 1;
        currentNode = MMU_read8(cpu->state, cpu->proc_ID, treeStart+pos+1);
      } else {
        // left
        if(currentNode & 0x80)
          writeData = 1;
        currentNode = MMU_read8(cpu->state, cpu->proc_ID, treeStart+pos);
      }
      
      if(writeData) {
        if(halfLen == 0)
          value |= currentNode;
        else
          value |= (currentNode<<4);

        halfLen += 4;
        if(halfLen == 8) {
          writeValue |= (value << byteShift);
          byteCount++;
          byteShift += 8;
          
          halfLen = 0;
          value = 0;

          if(byteCount == 4) {
            byteCount = 0;
            byteShift = 0;
            MMU_write8(cpu->state, cpu->proc_ID, dest, writeValue);
            dest += 4;
            writeValue = 0;
            len -= 4;
          }
        }
        pos = 0;
        currentNode = rootNode;
        writeData = 0;
      }
      mask >>= 1;
      if(mask == 0) {
        mask = 0x80000000;
        data = MMU_read8(cpu->state, cpu->proc_ID, source);
        source += 4;
      }
    }    
  }
  return 1;
}

u32 BitUnPack(armcpu_t* cpu)
{
  u32 source,dest,header,base,d,temp;
  int len,bits,revbits,dataSize,data,bitwritecount,mask,bitcount,addBase;
  u8 b;

  source = cpu->R[0];
  dest = cpu->R[1];
  header = cpu->R[2];
  
  len = MMU_read16(cpu->state, cpu->proc_ID, header);
  // check address
  bits = MMU_read8(cpu->state, cpu->proc_ID, header+2);
  revbits = 8 - bits; 
  // u32 value = 0;
  base = MMU_read8(cpu->state, cpu->proc_ID, header+4);
  addBase = (base & 0x80000000) ? 1 : 0;
  base &= 0x7fffffff;
  dataSize = MMU_read8(cpu->state, cpu->proc_ID, header+3);

  data = 0; 
  bitwritecount = 0; 
  while(1) {
    len -= 1;
    if(len < 0)
      break;
    mask = 0xff >> revbits; 
    b = MMU_read8(cpu->state, cpu->proc_ID, source);
    source++;
    bitcount = 0;
    while(1) {
      if(bitcount >= 8)
        break;
      d = b & mask;
      temp = d >> bitcount;
      if(!temp && addBase) {
        temp += base;
      }
      data |= temp << bitwritecount;
      bitwritecount += dataSize;
      if(bitwritecount >= 32) {
        MMU_write8(cpu->state, cpu->proc_ID, dest, data);
        dest += 4;
        data = 0;
        bitwritecount = 0;
      }
      mask <<= bits;
      bitcount += bits;
    }
  }
  return 1;
}

u32 Diff8bitUnFilterWram(armcpu_t* cpu)
{
  u32 source,dest,header;
  u8 data,diff;
  int len;

  source = cpu->R[0];
  dest = cpu->R[1];

  header = MMU_read8(cpu->state, cpu->proc_ID, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     (( (source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0))
    return 0;  

  len = header >> 8;

  data = MMU_read8(cpu->state, cpu->proc_ID, source++);
  MMU_write8(cpu->state, cpu->proc_ID, dest++, data);
  len--;
  
  while(len > 0) {
    diff = MMU_read8(cpu->state, cpu->proc_ID, source++);
    data += diff;
    MMU_write8(cpu->state, cpu->proc_ID, dest++, data);
    len--;
  }
  return 1;
}

u32 Diff16bitUnFilter(armcpu_t* cpu)
{
  u32 source,dest,header;
  u16 data;
  int len;

  source = cpu->R[0];
  dest = cpu->R[1];

  header = MMU_read8(cpu->state, cpu->proc_ID, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return 0;  
  
  len = header >> 8;

  data = MMU_read16(cpu->state, cpu->proc_ID, source);
  source += 2;
  MMU_write16(cpu->state, cpu->proc_ID, dest, data);
  dest += 2;
  len -= 2;
  
  while(len >= 2) {
    u16 diff = MMU_read16(cpu->state, cpu->proc_ID, source);
    source += 2;
    data += diff;
    MMU_write16(cpu->state, cpu->proc_ID, dest, data);
    dest += 2;
    len -= 2;
  }
  return 1;
}

u32 bios_sqrt(armcpu_t* cpu)
{
     cpu->R[0] = isqrt32(cpu->R[0]);
     return 1;
}

u32 setHaltCR(armcpu_t* cpu)
{ 
     MMU_write8(cpu->state, cpu->proc_ID, 0x4000300+cpu->proc_ID, cpu->R[0]);
     return 1;
}

u32 getSineTab(armcpu_t* cpu)
{ 
     cpu->R[0] = getsinetbl[cpu->R[0]];
     return 1;
}

u32 getPitchTab(armcpu_t* cpu)
{ 
     cpu->R[0] = getpitchtbl[cpu->R[0]];
     return 1;
}

u32 getVolumeTab(armcpu_t* cpu)
{ 
     cpu->R[0] = getvoltbl[cpu->R[0]];
     return 1;
}

u32 getCRC16(armcpu_t* cpu)
{
  unsigned int i,j;
  
  u32 crc = cpu->R[0];
  u32 datap = cpu->R[1];
  u32 size = cpu->R[2];
  
  static u16 val[] = { 0xC0C1,0xC181,0xC301,0xC601,0xCC01,0xD801,0xF001,0xA001 };
  for(i = 0; i < size; i++)
  {
    crc = crc ^ MMU_read8( cpu->state, cpu->proc_ID, datap + i);

    for(j = 0; j < 8; j++) {
      int do_bit = 0;

      if ( crc & 0x1)
        do_bit = 1;

      crc = crc >> 1;

      if ( do_bit) {
        crc = crc ^ (val[j] << (7-j));
      }
    }
  }
  cpu->R[0] = crc;
  return 1;
}

u32 SoundBias(armcpu_t* cpu)
{
 //    u32 current = SPU_ReadLong(0x4000504);
 //    if (cpu->R[0] > current)
	//SPU_WriteLong(0x4000504, current + 0x1);
 //    else
	//SPU_WriteLong(0x4000504, current - 0x1);
 //    return cpu->R[1];

     u32 curBias = MMU_read32(cpu->state, ARMCPU_ARM7,0x04000504);
	 u32 newBias = (curBias == 0) ? 0x000:0x200;
	 u32 delay = (newBias > curBias) ? (newBias-curBias) : (curBias-newBias);

	 MMU_write32(cpu->state, ARMCPU_ARM7,0x04000504, newBias);
     return cpu->R[1] * delay;
}

u32 (* ARM9_swi_tab[32])(armcpu_t* cpu)={
         bios_nop,             // 0x00
         bios_nop,             // 0x01
         bios_nop,             // 0x02
         delayLoop,            // 0x03
         intrWaitARM,          // 0x04
         waitVBlankARM,        // 0x05
         wait4IRQ,             // 0x06
         bios_nop,             // 0x07
         bios_nop,             // 0x08
         devide,               // 0x09
         bios_nop,             // 0x0A
         copy,                 // 0x0B
         fastCopy,             // 0x0C
         bios_sqrt,            // 0x0D
         getCRC16,             // 0x0E
         bios_nop,             // 0x0F
         BitUnPack,            // 0x10
         LZ77UnCompWram,       // 0x11
         LZ77UnCompVram,       // 0x12
         UnCompHuffman,        // 0x13
         RLUnCompWram,         // 0x14
         RLUnCompVram,         // 0x15
         Diff8bitUnFilterWram, // 0x16
         bios_nop,             // 0x17
         Diff16bitUnFilter,    // 0x18
         bios_nop,             // 0x19
         bios_nop,             // 0x1A
         bios_nop,             // 0x1B
         bios_nop,             // 0x1C
         bios_nop,             // 0x1D
         bios_nop,             // 0x1E
         setHaltCR,            // 0x1F
};

u32 (* ARM7_swi_tab[32])(armcpu_t* cpu)={
         bios_nop,             // 0x00
         bios_nop,             // 0x01
         bios_nop,             // 0x02
         delayLoop,            // 0x03
         intrWaitARM,          // 0x04
         waitVBlankARM,        // 0x05
         wait4IRQ,             // 0x06
         wait4IRQ,             // 0x07
         SoundBias,            // 0x08
         devide,               // 0x09
         bios_nop,             // 0x0A
         copy,                 // 0x0B
         fastCopy,             // 0x0C
         bios_sqrt,            // 0x0D
         getCRC16,             // 0x0E
         bios_nop,             // 0x0F
         BitUnPack,            // 0x10
         LZ77UnCompWram,       // 0x11
         LZ77UnCompVram,       // 0x12
         UnCompHuffman,        // 0x13
         RLUnCompWram,         // 0x14
         RLUnCompVram,         // 0x15
         Diff8bitUnFilterWram, // 0x16
         bios_nop,             // 0x17
         bios_nop,             // 0x18
         bios_nop,             // 0x19
         getSineTab,           // 0x1A
         getPitchTab,          // 0x1B
         getVolumeTab,         // 0x1C
         bios_nop,             // 0x1D
         bios_nop,             // 0x1E
         setHaltCR,            // 0x1F
};
