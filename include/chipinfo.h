/* Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 *  with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _CHIPINFO_H
#define _CHIPINFO_H
#define CHIPINFO_VERSION(major,minor) (((major)<<16)|(minor))
#define CHIPINFO_VERSION_UNKNOWN 0
#define CHIPINFO_PROTOCOL_REVISION 0x0000000000010002
#define CHIPINFO_PROTOCOL_GUID {0x61224FBE, 0xB240, 0x4D53, {0xB6, 0x61, 0xA9, 0xA1, 0xF3, 0x43, 0xB0, 0x5C}}
#define CHIPINFO_MAX_ID_LENGTH 16
#define CHIPINFO_MAX_NAME_LENGTH CHIPINFO_MAX_ID_LENGTH
#define CHIPINFO_MAX_CPU_CLUSTERS 4
#define CHIPINFO_RAW_VERSION_UNKNOWN        0x0
#define CHIPINFO_RAW_ID_UNKNOWN             0x0
#define CHIPINFO_SERIAL_NUM_UNKNOWN         0x0
#define CHIPINFO_RAW_DEVICE_FAMILY_UNKNOWN  0x0
#define CHIPINFO_RAW_DEVICE_NUMBER_UNKNOWN  0x0
#define CHIPINFO_QFPROM_CHIPID_UNKNOWN      0x0
typedef UINT32 ChipInfoVersionType;
typedef UINT32 ChipInfoModemType;
typedef enum{
	CHIPINFO_ID_UNKNOWN            = 0,
	CHIPINFO_ID_MDM1000            = 1,
	CHIPINFO_ID_ESM6235            = 2,
	CHIPINFO_ID_QSC6240            = 3,
	CHIPINFO_ID_MSM6245            = 4,
	CHIPINFO_ID_MSM6255            = 5,
	CHIPINFO_ID_MSM6255A           = 6,
	CHIPINFO_ID_MSM6260            = 7,
	CHIPINFO_ID_MSM6246            = 8,
	CHIPINFO_ID_QSC6270            = 9,
	CHIPINFO_ID_MSM6280            = 10,
	CHIPINFO_ID_MSM6290            = 11,
	CHIPINFO_ID_MSM7200            = 12,
	CHIPINFO_ID_MSM7201            = 13,
	CHIPINFO_ID_ESM7205            = 14,
	CHIPINFO_ID_ESM7206            = 15,
	CHIPINFO_ID_MSM7200A           = 16,
	CHIPINFO_ID_MSM7201A           = 17,
	CHIPINFO_ID_ESM7205A           = 18,
	CHIPINFO_ID_ESM7206A           = 19,
	CHIPINFO_ID_ESM7225            = 20,
	CHIPINFO_ID_MSM7225            = 21,
	CHIPINFO_ID_MSM7500            = 22,
	CHIPINFO_ID_MSM7500A           = 23,
	CHIPINFO_ID_MSM7525            = 24,
	CHIPINFO_ID_MSM7600            = 25,
	CHIPINFO_ID_MSM7601            = 26,
	CHIPINFO_ID_MSM7625            = 27,
	CHIPINFO_ID_MSM7800            = 28,
	CHIPINFO_ID_MDM8200            = 29,
	CHIPINFO_ID_QSD8650            = 30,
	CHIPINFO_ID_MDM8900            = 31,
	CHIPINFO_ID_QST1000            = 32,
	CHIPINFO_ID_QST1005            = 33,
	CHIPINFO_ID_QST1100            = 34,
	CHIPINFO_ID_QST1105            = 35,
	CHIPINFO_ID_QST1500            = 40,
	CHIPINFO_ID_QST1600            = 41,
	CHIPINFO_ID_QST1700            = 42,
	CHIPINFO_ID_QSD8250            = 36,
	CHIPINFO_ID_QSD8550            = 37,
	CHIPINFO_ID_QSD8850            = 38,
	CHIPINFO_ID_MDM2000            = 39,
	CHIPINFO_ID_MSM7227            = 43,
	CHIPINFO_ID_MSM7627            = 44,
	CHIPINFO_ID_QSC6165            = 45,
	CHIPINFO_ID_QSC6175            = 46,
	CHIPINFO_ID_QSC6185            = 47,
	CHIPINFO_ID_QSC6195            = 48,
	CHIPINFO_ID_QSC6285            = 49,
	CHIPINFO_ID_QSC6295            = 50,
	CHIPINFO_ID_QSC6695            = 51,
	CHIPINFO_ID_ESM6246            = 52,
	CHIPINFO_ID_ESM6290            = 53,
	CHIPINFO_ID_ESC6270            = 54,
	CHIPINFO_ID_ESC6240            = 55,
	CHIPINFO_ID_MDM8220            = 56,
	CHIPINFO_ID_MDM9200            = 57,
	CHIPINFO_ID_MDM9600            = 58,
	CHIPINFO_ID_MSM7630            = 59,
	CHIPINFO_ID_MSM7230            = 60,
	CHIPINFO_ID_ESM7227            = 61,
	CHIPINFO_ID_MSM7625D1          = 62,
	CHIPINFO_ID_MSM7225D1          = 63,
	CHIPINFO_ID_QSD8250A           = 64,
	CHIPINFO_ID_QSD8650A           = 65,
	CHIPINFO_ID_MSM7625D2          = 66,
	CHIPINFO_ID_MSM7227D1          = 67,
	CHIPINFO_ID_MSM7627D1          = 68,
	CHIPINFO_ID_MSM7627D2          = 69,
	CHIPINFO_ID_MSM8260            = 70,
	CHIPINFO_ID_MSM8660            = 71,
	CHIPINFO_ID_MDM8200A           = 72,
	CHIPINFO_ID_QSC6155            = 73,
	CHIPINFO_ID_MSM8255            = 74,
	CHIPINFO_ID_MSM8655            = 75,
	CHIPINFO_ID_ESC6295            = 76,
	CHIPINFO_ID_MDM3000            = 77,
	CHIPINFO_ID_MDM6200            = 78,
	CHIPINFO_ID_MDM6600            = 79,
	CHIPINFO_ID_MDM6210            = 80,
	CHIPINFO_ID_MDM6610            = 81,
	CHIPINFO_ID_QSD8672            = 82,
	CHIPINFO_ID_MDM6215            = 83,
	CHIPINFO_ID_MDM6615            = 84,
	CHIPINFO_ID_APQ8055            = 85,
	CHIPINFO_ID_APQ8060            = 86,
	CHIPINFO_ID_MSM8960            = 87,
	CHIPINFO_ID_MSM7225A           = 88,
	CHIPINFO_ID_MSM7625A           = 89,
	CHIPINFO_ID_MSM7227A           = 90,
	CHIPINFO_ID_MSM7627A           = 91,
	CHIPINFO_ID_ESM7227A           = 92,
	CHIPINFO_ID_QSC6195D2          = 93,
	CHIPINFO_ID_FSM9200            = 94,
	CHIPINFO_ID_FSM9800            = 95,
	CHIPINFO_ID_MSM7225AD1         = 96,
	CHIPINFO_ID_MSM7227AD1         = 97,
	CHIPINFO_ID_MSM7225AA          = 98,
	CHIPINFO_ID_MSM7225AAD1        = 99,
	CHIPINFO_ID_MSM7625AA          = 100,
	CHIPINFO_ID_MSM7227AA          = 101,
	CHIPINFO_ID_MSM7227AAD1        = 102,
	CHIPINFO_ID_MSM7627AA          = 103,
	CHIPINFO_ID_MDM9615            = 104,
	CHIPINFO_ID_MDM9615M           = CHIPINFO_ID_MDM9615,
	CHIPINFO_ID_MDM8215            = 106,
	CHIPINFO_ID_MDM9215            = 107,
	CHIPINFO_ID_MDM9215M           = CHIPINFO_ID_MDM9215,
	CHIPINFO_ID_APQ8064            = 109,
	CHIPINFO_ID_QSC6270D1          = 110,
	CHIPINFO_ID_QSC6240D1          = 111,
	CHIPINFO_ID_ESC6270D1          = 112,
	CHIPINFO_ID_ESC6240D1          = 113,
	CHIPINFO_ID_MDM6270            = 114,
	CHIPINFO_ID_MDM6270D1          = 115,
	CHIPINFO_ID_MSM8930            = 116,
	CHIPINFO_ID_MSM8630            = 117,
	CHIPINFO_ID_MSM8230            = 118,
	CHIPINFO_ID_APQ8030            = 119,
	CHIPINFO_ID_MSM8627            = 120,
	CHIPINFO_ID_MSM8227            = 121,
	CHIPINFO_ID_MSM8660A           = 122,
	CHIPINFO_ID_MSM8260A           = 123,
	CHIPINFO_ID_APQ8060A           = 124,
	CHIPINFO_ID_MPQ8062            = 125,
	CHIPINFO_ID_MSM8974            = 126,
	CHIPINFO_ID_MSM8225            = 127,
	CHIPINFO_ID_MSM8225D1          = 128,
	CHIPINFO_ID_MSM8625            = 129,
	CHIPINFO_ID_MPQ8064            = 130,
	CHIPINFO_ID_MSM7225AB          = 131,
	CHIPINFO_ID_MSM7225ABD1        = 132,
	CHIPINFO_ID_MSM7625AB          = 133,
	CHIPINFO_ID_MDM9625            = 134,
	CHIPINFO_ID_MSM7125A           = 135,
	CHIPINFO_ID_MSM7127A           = 136,
	CHIPINFO_ID_MSM8125AB          = 137,
	CHIPINFO_ID_MSM8626            = 145,
	CHIPINFO_ID_MPQ8092            = 146,
	CHIPINFO_ID_MSM8610            = 147,
	CHIPINFO_ID_MDM8225            = 148,
	CHIPINFO_ID_MDM9225            = 149,
	CHIPINFO_ID_MDM9225M           = 150,
	CHIPINFO_ID_MDM9624M           = 151,
	CHIPINFO_ID_MDM9625M           = 152,
	CHIPINFO_ID_MSM8226            = 158,
	CHIPINFO_ID_MSM8826            = 159,
	CHIPINFO_ID_APQ8030AA          = 160,
	CHIPINFO_ID_MSM8110            = 161,
	CHIPINFO_ID_MSM8210            = 162,
	CHIPINFO_ID_MSM8810            = 163,
	CHIPINFO_ID_MSM8212            = 164,
	CHIPINFO_ID_MSM8612            = 165,
	CHIPINFO_ID_MSM8812            = 166,
	CHIPINFO_ID_MSM8125            = 167,
	CHIPINFO_ID_MSM8225Q           = 168,
	CHIPINFO_ID_MSM8625Q           = 169,
	CHIPINFO_ID_MSM8125Q           = 170,
	CHIPINFO_ID_MDM9310            = 171,
	CHIPINFO_ID_APQ8064_SLOW_PRIME = 172,
	CHIPINFO_ID_MDM8110M           = 173,
	CHIPINFO_ID_MDM8615M           = 174,
	CHIPINFO_ID_MDM9320            = 175,
	CHIPINFO_ID_MDM9225_1          = 176,
	CHIPINFO_ID_MDM9225M_1         = 177,
	CHIPINFO_ID_APQ8084            = 178,
	CHIPINFO_ID_MSM8130            = 179,
	CHIPINFO_ID_MSM8130AA          = 180,
	CHIPINFO_ID_MSM8130AB          = 181,
	CHIPINFO_ID_MSM8627AA          = 182,
	CHIPINFO_ID_MSM8227AA          = 183,
	CHIPINFO_ID_APQ8074            = 184,
	CHIPINFO_ID_MSM8274            = 185,
	CHIPINFO_ID_MSM8674            = 186,
	CHIPINFO_ID_MDM9635            = 187,
	CHIPINFO_ID_FSM9900            = 188,
	CHIPINFO_ID_FSM9965            = 189,
	CHIPINFO_ID_FSM9955            = 190,
	CHIPINFO_ID_FSM9950            = 191,
	CHIPINFO_ID_FSM9915            = 192,
	CHIPINFO_ID_FSM9910            = 193,
	CHIPINFO_ID_MSM8974_PRO        = 194,
	CHIPINFO_ID_MSM8962            = 195,
	CHIPINFO_ID_MSM8262            = 196,
	CHIPINFO_ID_APQ8062            = 197,
	CHIPINFO_ID_MSM8126            = 198,
	CHIPINFO_ID_APQ8026            = 199,
	CHIPINFO_ID_MSM8926            = 200,
	CHIPINFO_ID_MSM8326            = 205,
	CHIPINFO_ID_MSM8916            = 206,
	CHIPINFO_ID_MSM8994            = 207,
	CHIPINFO_ID_APQ8074_AA         = 208,
	CHIPINFO_ID_APQ8074_AB         = 209,
	CHIPINFO_ID_APQ8074_PRO        = 210,
	CHIPINFO_ID_MSM8274_AA         = 211,
	CHIPINFO_ID_MSM8274_AB         = 212,
	CHIPINFO_ID_MSM8274_PRO        = 213,
	CHIPINFO_ID_MSM8674_AA         = 214,
	CHIPINFO_ID_MSM8674_AB         = 215,
	CHIPINFO_ID_MSM8674_PRO        = 216,
	CHIPINFO_ID_MSM8974_AA         = 217,
	CHIPINFO_ID_MSM8974_AB         = 218,
	CHIPINFO_ID_APQ8028            = 219,
	CHIPINFO_ID_MSM8128            = 220,
	CHIPINFO_ID_MSM8228            = 221,
	CHIPINFO_ID_MSM8528            = 222,
	CHIPINFO_ID_MSM8628            = 223,
	CHIPINFO_ID_MSM8928            = 224,
	CHIPINFO_ID_MSM8510            = 225,
	CHIPINFO_ID_MSM8512            = 226,
	CHIPINFO_ID_MDM9630            = 227,
	CHIPINFO_ID_MDM9635M           = CHIPINFO_ID_MDM9635,
	CHIPINFO_ID_MDM9230            = 228,
	CHIPINFO_ID_MDM9235M           = 229,
	CHIPINFO_ID_MDM8630            = 230,
	CHIPINFO_ID_MDM9330            = 231,
	CHIPINFO_ID_MPQ8091            = 232,
	CHIPINFO_ID_MSM8936            = 233,
	CHIPINFO_ID_MDM9240            = 234,
	CHIPINFO_ID_MDM9340            = 235,
	CHIPINFO_ID_MDM9640            = 236,
	CHIPINFO_ID_MDM9245M           = 237,
	CHIPINFO_ID_MDM9645M           = 238,
	CHIPINFO_ID_MSM8939            = 239,
	CHIPINFO_ID_APQ8036            = 240,
	CHIPINFO_ID_APQ8039            = 241,
	CHIPINFO_ID_MSM8236            = 242,
	CHIPINFO_ID_MSM8636            = 243,
	CHIPINFO_ID_APQ8064_AU         = 244,
	CHIPINFO_ID_MSM8909            = 245,
	CHIPINFO_ID_MSM8996            = 246,
	CHIPINFO_ID_APQ8016            = 247,
	CHIPINFO_ID_MSM8216            = 248,
	CHIPINFO_ID_MSM8116            = 249,
	CHIPINFO_ID_MSM8616            = 250,
	CHIPINFO_ID_MSM8992            = 251,
	CHIPINFO_ID_APQ8092            = 252,
	CHIPINFO_ID_APQ8094            = 253,
	CHIPINFO_ID_FSM9008            = 254,
	CHIPINFO_ID_FSM9010            = 255,
	CHIPINFO_ID_FSM9016            = 256,
	CHIPINFO_ID_FSM9055            = 257,
	CHIPINFO_ID_MSM8209            = 258,
	CHIPINFO_ID_MSM8208            = 259,
	CHIPINFO_ID_MDM9209            = 260,
	CHIPINFO_ID_MDM9309            = 261,
	CHIPINFO_ID_MDM9609            = 262,
	CHIPINFO_ID_MSM8239            = 263,
	CHIPINFO_ID_MSM8952            = 264,
	CHIPINFO_ID_APQ8009            = 265,
	CHIPINFO_ID_MSM8956            = 266,
	CHIPINFO_ID_QDF2432            = 267,
	CHIPINFO_ID_MSM8929            = 268,
	CHIPINFO_ID_MSM8629            = 269,
	CHIPINFO_ID_MSM8229            = 270,
	CHIPINFO_ID_APQ8029            = 271,
	CHIPINFO_ID_QCA9618            = 272,
	CHIPINFO_ID_IPQ4018            = CHIPINFO_ID_QCA9618,
	CHIPINFO_ID_QCA9619            = 273,
	CHIPINFO_ID_IPQ4019            = CHIPINFO_ID_QCA9619,
	CHIPINFO_ID_APQ8056            = 274,
	CHIPINFO_ID_MSM8609            = 275,
	CHIPINFO_ID_FSM9916            = 276,
	CHIPINFO_ID_APQ8076            = 277,
	CHIPINFO_ID_MSM8976            = 278,
	CHIPINFO_ID_MDM9650            = 279,
	CHIPINFO_ID_IPQ8065            = 280,
	CHIPINFO_ID_IPQ8069            = 281,
	CHIPINFO_ID_MSM8939_BC         = 282,
	CHIPINFO_ID_MDM9250            = 283,
	CHIPINFO_ID_MDM9255            = 284,
	CHIPINFO_ID_MDM9350            = 285,
	CHIPINFO_ID_MDM9655            = 286,
	CHIPINFO_ID_IPQ4028            = 287,
	CHIPINFO_ID_IPQ4029            = 288,
	CHIPINFO_ID_APQ8052            = 289,
	CHIPINFO_ID_MDM9607            = 290,
	CHIPINFO_ID_APQ8096            = 291,
	CHIPINFO_ID_MSM8998            = 292,
	CHIPINFO_ID_MSM8953            = 293,
	CHIPINFO_ID_MSM8937            = 294,
	CHIPINFO_ID_APQ8037            = 295,
	CHIPINFO_ID_MDM8207            = 296,
	CHIPINFO_ID_MDM9207            = 297,
	CHIPINFO_ID_MDM9307            = 298,
	CHIPINFO_ID_MDM9628            = 299,
	CHIPINFO_ID_MSM8909W           = 300,
	CHIPINFO_ID_APQ8009W           = 301,
	CHIPINFO_ID_MSM8996L           = 302,
	CHIPINFO_ID_MSM8917            = 303,
	CHIPINFO_ID_APQ8053            = 304,
	CHIPINFO_ID_MSM8996SG          = 305,
	CHIPINFO_ID_MSM8997            = 306,
	CHIPINFO_ID_APQ8017            = 307,
	CHIPINFO_ID_MSM8217            = 308,
	CHIPINFO_ID_MSM8617            = 309,
	CHIPINFO_ID_MSM8996AU          = 310,
	CHIPINFO_ID_APQ8096AU          = 311,
	CHIPINFO_ID_APQ8096SG          = 312,
	CHIPINFO_ID_MSM8940            = 313,
	CHIPINFO_ID_MDM9665            = 314,
	CHIPINFO_ID_MSM8996SGAU        = 315,
	CHIPINFO_ID_APQ8096SGAU        = 316,
	CHIPINFO_ID_SDM660             = 317,
	CHIPINFO_ID_SDM630             = 318,
	CHIPINFO_ID_APQ8098            = 319,
	CHIPINFO_ID_MSM8920            = 320,
	CHIPINFO_ID_SDM845             = 321,
	CHIPINFO_ID_MDM9206            = 322,
	CHIPINFO_ID_IPQ8074            = 323,
	CHIPINFO_ID_SDA660             = 324,
	CHIPINFO_ID_SDM658             = 325,
	CHIPINFO_ID_SDA658             = 326,
	CHIPINFO_ID_SDA630             = 327,
	CHIPINFO_ID_SDM830             = 328,
	CHIPINFO_ID_SDX50M             = 329,
	CHIPINFO_ID_QCA6290            = 330,
	CHIPINFO_ID_MSM8905            = 331,
	CHIPINFO_ID_SDC830             = 332,
	CHIPINFO_ID_MDM9660            = 333,
	CHIPINFO_ID_SDX24              = 334,
	CHIPINFO_ID_SDX24M             = 335,
	CHIPINFO_ID_SDM670             = 336,
	CHIPINFO_ID_SDA670             = 337,
	CHIPINFO_ID_SDM450             = 338,
	CHIPINFO_ID_SDM855             = 339,
	CHIPINFO_ID_SDM900             = 340,
	CHIPINFO_ID_SDM1000            = CHIPINFO_ID_SDM900,
	CHIPINFO_ID_SDC845             = 341,
	CHIPINFO_ID_IPQ8072            = 342,
	CHIPINFO_ID_IPQ8076            = 343,
	CHIPINFO_ID_IPQ8078            = 344,
	CHIPINFO_ID_SDA845             = CHIPINFO_ID_SDC845,
	CHIPINFO_ID_SDX20M             = CHIPINFO_ID_MDM9665,
	CHIPINFO_ID_SDM636             = 345,
	CHIPINFO_ID_SDA636             = 346,
	CHIPINFO_ID_QCS605             = 347,
	CHIPINFO_ID_SDM850             = 348,
	CHIPINFO_ID_SDM632             = 349,
	CHIPINFO_ID_SDA632             = 350,
	CHIPINFO_ID_SDA450             = 351,
	CHIPINFO_ID_QCS6200            = 352,
	CHIPINFO_ID_SDM439             = 353,
	CHIPINFO_ID_SDM429             = 354,
	CHIPINFO_ID_SDM640             = 355,
	CHIPINFO_ID_QCS405             = CHIPINFO_ID_QCS6200,
	CHIPINFO_ID_SDA865             = 356,
	CHIPINFO_ID_SDX55              = 357,
	CHIPINFO_ID_QCA6390            = 358,
	CHIPINFO_ID_MDM9150            = 359,
	CHIPINFO_ID_SDM710             = 360,
	CHIPINFO_ID_SDA855             = 361,
	CHIPINFO_ID_SDM855A            = 362,
	CHIPINFO_ID_SM8150             = CHIPINFO_ID_SDM855,
	CHIPINFO_ID_SM8150P            = CHIPINFO_ID_SDA855,
	CHIPINFO_ID_SM8250             = CHIPINFO_ID_SDA865,
	CHIPINFO_ID_SCX8180            = CHIPINFO_ID_SDM1000,
	CHIPINFO_ID_SDA439             = 363,
	CHIPINFO_ID_SDA429             = 364,
	CHIPINFO_ID_SM7150             = 365,
	CHIPINFO_ID_SM7150P            = 366,
	CHIPINFO_ID_SDA855A            = 367,
	CHIPINFO_ID_SA8150             = CHIPINFO_ID_SDM855A,
	CHIPINFO_ID_SA8150P            = CHIPINFO_ID_SDA855A,
	CHIPINFO_ID_SDX55M             = 368,
	CHIPINFO_ID_SM6150             = CHIPINFO_ID_SDM640,
	CHIPINFO_ID_SM6150P            = 369,
	CHIPINFO_ID_SXR1120            = 370,
	CHIPINFO_ID_SXR1130            = 371,
	CHIPINFO_ID_QCS401             = 372,
	CHIPINFO_ID_QCS403             = 373,
	CHIPINFO_ID_SA8155             = 374,
	CHIPINFO_ID_IPQ8070            = 375,
	CHIPINFO_ID_IPQ8071            = 376,
	CHIPINFO_ID_SA6155P            = 377,
	CHIPINFO_ID_SA6150P            = 378,
	CHIPINFO_ID_SA6145P            = 379,
	CHIPINFO_ID_SA4155P            = 380,
	CHIPINFO_ID_QCA6595            = 381,
	CHIPINFO_ID_QCN7605            = 382,
	CHIPINFO_ID_QCN7606            = 383,
	CHIPINFO_ID_SC8180X            = CHIPINFO_ID_SCX8180,
	CHIPINFO_ID_SA6155             = 384,
	CHIPINFO_ID_SDM455             = 385,
	CHIPINFO_ID_QM215              = 386,
	CHIPINFO_ID_APQ8096A           = 387,
	CHIPINFO_ID_MDM9205            = 388,
	CHIPINFO_ID_IPQ8072A           = 389,
	CHIPINFO_ID_IPQ8074A           = 390,
	CHIPINFO_ID_IPQ8076A           = 391,
	CHIPINFO_ID_IPQ8078A           = 392,
	CHIPINFO_ID_SDM712             = 393,
	CHIPINFO_ID_SM_NICOBAR         = 394,
	CHIPINFO_ID_IPQ8070A           = 395,
	CHIPINFO_ID_IPQ8071A           = 396,
	CHIPINFO_ID_IPQ8172            = 397,
	CHIPINFO_ID_IPQ8173            = 398,
	CHIPINFO_ID_IPQ8174            = 399,
	CHIPINFO_ID_SM_SAIPAN          = 400,
	CHIPINFO_ID_QCS610             = 401,
	CHIPINFO_ID_IPQ6018            = 402,
	CHIPINFO_ID_IPQ6028            = 403,
	CHIPINFO_ID_SC8180XP           = 404,
	CHIPINFO_ID_SA8195P            = 405,
	CHIPINFO_ID_QCS410             = 406,
	CHIPINFO_ID_SM_RENNELL         = 407,
	CHIPINFO_ID_SA415M             = 408,
	CHIPINFO_ID_QCA6490            = 409,
	CHIPINFO_ID_QCS404             = 410,
	CHIPINFO_ID_QCS407             = 411,
	CHIPINFO_ID_QCA6480            = 412,
	CHIPINFO_ID_QCA6481            = 413,
	CHIPINFO_ID_QCA6491            = 414,
	CHIPINFO_ID_SM_LAHAINA         = 415,
	CHIPINFO_ID_SDM429W            = 416,
	CHIPINFO_ID_SM_KAMORTA         = 417,
	CHIPINFO_ID_SA515M             = 418,
	CHIPINFO_ID_FSM10055           = 419,
	CHIPINFO_ID_SMP_KAMORTA        = 420,
	CHIPINFO_ID_IPQ6000            = 421,
	CHIPINFO_ID_IPQ6010            = 422,
	CHIPINFO_ID_QCN3018            = 423,
	CHIPINFO_ID_SMP_RENNELL        = 424,
	CHIPINFO_ID_SC7180             = 425,
	CHIPINFO_ID_QCN9000            = 426,
	CHIPINFO_ID_QCN9001            = 427,
	CHIPINFO_ID_QCN9002            = 428,
	CHIPINFO_ID_QCN9003            = 429,
	CHIPINFO_ID_QCN9010            = 430,
	CHIPINFO_ID_QCN9011            = 431,
	CHIPINFO_ID_QCN9012            = 432,
	CHIPINFO_ID_QCN9013            = 433,
	CHIPINFO_ID_SM_BITRA           = 434,
	CHIPINFO_ID_SMP_BITRA          = 435,
	CHIPINFO_ID_QCM2150            = 436,
	CHIPINFO_ID_SDA429W            = 437,
	CHIPINFO_ID_SDX_CHITWAN        = 438,
	CHIPINFO_ID_SMP_LAHAINA        = 439,
	CHIPINFO_ID_SM_SAIPAN_MODULE   = 440,
	CHIPINFO_ID_QM_AGATTI          = 441,
	CHIPINFO_ID_WCN_MOSELLE        = 442,
	CHIPINFO_ID_SM_RENNELL_AB      = 443,
	CHIPINFO_ID_SM_KAMORTA_H       = 444,
	CHIPINFO_ID_SMP_KAMORTA_H      = 445,
	CHIPINFO_ID_IPQ5010            = 446,
	CHIPINFO_ID_IPQ5018            = 447,
	CHIPINFO_ID_IPQ5028            = 448,
	CHIPINFO_ID_SC_MAKENA          = 449,
	CHIPINFO_ID_SM_CEDROS          = 450,
	CHIPINFO_ID_SA2145P            = 451,
	CHIPINFO_ID_SA2150P            = 452,
	CHIPINFO_ID_IPQ6005            = 453,
	CHIPINFO_ID_SM_MANNAR          = 454,
	CHIPINFO_ID_QRB5165            = 455,
	CHIPINFO_ID_SM_LAHAINA_MODULE  = 456,
	CHIPINFO_ID_SM_WAIPIO          = 457,
	CHIPINFO_ID_SDX_OLYMPIC        = 458,
	CHIPINFO_ID_SM_BITRA_H         = 459,
	CHIPINFO_ID_SA_MAKENA_IVI      = 460,
	CHIPINFO_ID_SA_MAKENA_ADAS     = 461,
	CHIPINFO_ID_FSM10000           = 462,
	CHIPINFO_ID_FSM10005           = 463,
	CHIPINFO_ID_FSM10010           = 464,
	CHIPINFO_ID_FSM10051           = 465,
	CHIPINFO_ID_FSM10056           = 466,
	CHIPINFO_ID_QCM_NICOBAR        = 467,
	CHIPINFO_ID_QCS_NICOBAR        = 468,
	CHIPINFO_ID_QCM_KAMORTA        = 469,
	CHIPINFO_ID_QCS_KAMORTA        = 470,
	CHIPINFO_ID_QMP_AGATTI         = 471,
	CHIPINFO_ID_SM_AGATTI          = CHIPINFO_ID_QM_AGATTI,
	CHIPINFO_ID_SMP_AGATTI         = CHIPINFO_ID_QMP_AGATTI,
	CHIPINFO_ID_SM_MANNAR_H        = 472,
	CHIPINFO_ID_SCP_MAKENA         = CHIPINFO_ID_SC_MAKENA,
	CHIPINFO_ID_QCM_AGATTI         = 473,
	CHIPINFO_ID_QCS_AGATTI         = 474,
	CHIPINFO_ID_SM_KODIAK          = 475,
	CHIPINFO_ID_SM_FRASER          = 476,
	CHIPINFO_ID_QCN9100            = 477,
	CHIPINFO_ID_QCN9101            = 478,
	CHIPINFO_ID_QCN9102            = 479,
	CHIPINFO_ID_QCN9110            = 480,
	CHIPINFO_ID_QCS8250            = 481,
	CHIPINFO_ID_SMP_WAIPIO         = 482,
	CHIPINFO_ID_SDX_OLYMPIC_HYBRID = CHIPINFO_ID_SDX_OLYMPIC,
	CHIPINFO_ID_SDX_OLYMPIC_SINGLEDIE = 483,
	CHIPINFO_ID_SDX12              = 484,
	CHIPINFO_ID_WCN7850            = 485,
	CHIPINFO_ID_SDW_ATHERTON       = 486,
	CHIPINFO_ID_SC_KODIAK_CHROME   = 487,
	CHIPINFO_ID_SC_KODIAK_WINDOWS  = 488,
	CHIPINFO_ID_QCN9022            = 489,
	CHIPINFO_ID_QCN9024            = 490,
	CHIPINFO_ID_QCN9070            = 491,
	CHIPINFO_ID_QCN9072            = 492,
	CHIPINFO_ID_QCN9074            = 493,
	CHIPINFO_ID_QTANG2             = 494,
	CHIPINFO_ID_SC7180P            = 495,
	CHIPINFO_ID_QRB5165N           = 496,
	CHIPINFO_ID_QCM_KODIAK         = 497,
	CHIPINFO_ID_QCS_KODIAK         = 498,
	CHIPINFO_ID_SMP_KODIAK         = 499,
	CHIPINFO_ID_SDX57              = 500,
	CHIPINFO_ID_SM_LAHAINA_LTE_ONLY = 501,
	CHIPINFO_ID_SMP_LAHAINA_OEM_SPECIFIC = 502,
	CHIPINFO_ID_IPQ5000            = 503,
	CHIPINFO_ID_IPQ0509            = 504,
	CHIPINFO_ID_IPQ0518            = 505,
	CHIPINFO_ID_SM_FILLMORE        = 506,
	CHIPINFO_ID_SM_STRAIT          = 507,
	CHIPINFO_ID_WCN7851            = 508,
	CHIPINFO_ID_SDX_OLYMPIC_LITE   = 509,
	CHIPINFO_ID_IPQ9008            = 510,
	CHIPINFO_ID_IPQ9028            = 511,
	CHIPINFO_ID_IPQ9038            = 512,
	CHIPINFO_ID_IPQ9047            = 513,
	CHIPINFO_ID_IPQ9048            = 514,
	CHIPINFO_ID_SM_KODIAK_LTE_ONLY = 515,
	CHIPINFO_ID_QCX315             = 516,
	CHIPINFO_ID_SWP_ATHERTON       = 517,
	CHIPINFO_ID_SM_DIVAR           = 518,
	CHIPINFO_ID_SM_KAILUA          = 519,
	CHIPINFO_NUM_IDS,
	CHIPINFO_ID_32BITS = 0x7FFFFFF
}ChipInfoIdType;
typedef enum{
	CHIPINFO_FAMILY_UNKNOWN    = 0,
	CHIPINFO_FAMILY_MSM6246    = 1,
	CHIPINFO_FAMILY_MSM6260    = 2,
	CHIPINFO_FAMILY_QSC6270    = 3,
	CHIPINFO_FAMILY_MSM6280    = 4,
	CHIPINFO_FAMILY_MSM6290    = 5,
	CHIPINFO_FAMILY_MSM7200    = 6,
	CHIPINFO_FAMILY_MSM7500    = 7,
	CHIPINFO_FAMILY_MSM7600    = 8,
	CHIPINFO_FAMILY_MSM7625    = 9,
	CHIPINFO_FAMILY_MSM7X30    = 10,
	CHIPINFO_FAMILY_MSM7800    = 11,
	CHIPINFO_FAMILY_MDM8200    = 12,
	CHIPINFO_FAMILY_QSD8650    = 13,
	CHIPINFO_FAMILY_MSM7627    = 14,
	CHIPINFO_FAMILY_QSC6695    = 15,
	CHIPINFO_FAMILY_MDM9X00    = 16,
	CHIPINFO_FAMILY_QSD8650A   = 17,
	CHIPINFO_FAMILY_MSM8X60    = 18,
	CHIPINFO_FAMILY_MDM8200A   = 19,
	CHIPINFO_FAMILY_QSD8672    = 20,
	CHIPINFO_FAMILY_MDM6615    = 21,
	CHIPINFO_FAMILY_MSM8660    = CHIPINFO_FAMILY_MSM8X60,
	CHIPINFO_FAMILY_MSM8960    = 22,
	CHIPINFO_FAMILY_MSM7625A   = 23,
	CHIPINFO_FAMILY_MSM7627A   = 24,
	CHIPINFO_FAMILY_MDM9X15    = 25,
	CHIPINFO_FAMILY_MSM8930    = 26,
	CHIPINFO_FAMILY_MSM8630    = CHIPINFO_FAMILY_MSM8930,
	CHIPINFO_FAMILY_MSM8230    = CHIPINFO_FAMILY_MSM8930,
	CHIPINFO_FAMILY_APQ8030    = CHIPINFO_FAMILY_MSM8930,
	CHIPINFO_FAMILY_MSM8627    = 30,
	CHIPINFO_FAMILY_MSM8227    = CHIPINFO_FAMILY_MSM8627,
	CHIPINFO_FAMILY_MSM8974    = 32,
	CHIPINFO_FAMILY_MSM8625    = 33,
	CHIPINFO_FAMILY_MSM8225    = CHIPINFO_FAMILY_MSM8625,
	CHIPINFO_FAMILY_APQ8064    = 34,
	CHIPINFO_FAMILY_MDM9x25    = 35,
	CHIPINFO_FAMILY_MSM8960AB  = 36,
	CHIPINFO_FAMILY_MSM8930AB  = 36,
	CHIPINFO_FAMILY_MSM8x10    = 38,
	CHIPINFO_FAMILY_MPQ8092    = 39,
	CHIPINFO_FAMILY_MSM8x26    = 40,
	CHIPINFO_FAMILY_MSM8225Q   = 41,
	CHIPINFO_FAMILY_MSM8625Q   = 42,
	CHIPINFO_FAMILY_APQ8x94    = 43,
	CHIPINFO_FAMILY_MSM8x32    = 44,
	CHIPINFO_FAMILY_MDM9x35    = 45,
	CHIPINFO_FAMILY_MSM8974_PRO= 46,
	CHIPINFO_FAMILY_FSM9900    = 47,
	CHIPINFO_FAMILY_MSM8x62    = 48,
	CHIPINFO_FAMILY_MSM8926    = 49,
	CHIPINFO_FAMILY_MSM8994    = 50,
	CHIPINFO_FAMILY_IPQ8064    = 51,
	CHIPINFO_FAMILY_MSM8916    = 52,
	CHIPINFO_FAMILY_MSM8936    = 53,
	CHIPINFO_FAMILY_MDM9x45    = 54,
	CHIPINFO_FAMILY_MSM8996    = 56,
	CHIPINFO_FAMILY_APQ8096    = CHIPINFO_FAMILY_MSM8996,
	CHIPINFO_FAMILY_MSM8992    = 57,
	CHIPINFO_FAMILY_MSM8909    = 58,
	CHIPINFO_FAMILY_FSM90xx    = 59,
	CHIPINFO_FAMILY_MSM8952    = 60,
	CHIPINFO_FAMILY_QDF2432    = 61,
	CHIPINFO_FAMILY_MSM8929    = 62,
	CHIPINFO_FAMILY_MSM8956    = 63,
	CHIPINFO_FAMILY_MSM8976    = CHIPINFO_FAMILY_MSM8956,
	CHIPINFO_FAMILY_QCA961x    = 64,
	CHIPINFO_FAMILY_IPQ40xx    = CHIPINFO_FAMILY_QCA961x,
	CHIPINFO_FAMILY_MDM9x55    = 65,
	CHIPINFO_FAMILY_MDM9x07    = 66,
	CHIPINFO_FAMILY_MSM8998    = 67,
	CHIPINFO_FAMILY_MSM8953    = 68,
	CHIPINFO_FAMILY_MSM8993    = 69,
	CHIPINFO_FAMILY_MSM8937    = 70,
	CHIPINFO_FAMILY_MSM8917    = 71,
	CHIPINFO_FAMILY_MSM8996SG  = 72,
	CHIPINFO_FAMILY_MSM8997    = 73,
	CHIPINFO_FAMILY_MSM8940    = 74,
	CHIPINFO_FAMILY_MDM9x65    = 75,
	CHIPINFO_FAMILY_SDM660     = 76,
	CHIPINFO_FAMILY_SDM630     = 77,
	CHIPINFO_FAMILY_MSM8920    = 78,
	CHIPINFO_FAMILY_SDM845     = 79,
	CHIPINFO_FAMILY_IPQ807x    = 80,
	CHIPINFO_FAMILY_SDM830     = 81,
	CHIPINFO_FAMILY_SDX50M     = 82,
	CHIPINFO_FAMILY_QCA6290    = 83,
	CHIPINFO_FAMILY_SDX24      = 84,
	CHIPINFO_FAMILY_SDM670     = 85,
	CHIPINFO_FAMILY_SDM855     = 86,
	CHIPINFO_FAMILY_SDM900     = 87,
	CHIPINFO_FAMILY_SDM1000    = CHIPINFO_FAMILY_SDM900,
	CHIPINFO_FAMILY_SDX20M     = CHIPINFO_FAMILY_MDM9x65,
	CHIPINFO_FAMILY_QCS605     = 88,
	CHIPINFO_FAMILY_SDM632     = 89,
	CHIPINFO_FAMILY_QCS6200    = 90,
	CHIPINFO_FAMILY_SDM439     = 91,
	CHIPINFO_FAMILY_SDM640     = 92,
	CHIPINFO_FAMILY_QCS405     = CHIPINFO_FAMILY_QCS6200,
	CHIPINFO_FAMILY_SDA865     = 93,
	CHIPINFO_FAMILY_SDX55      = 94,
	CHIPINFO_FAMILY_QCA639x    = 95,
	CHIPINFO_FAMILY_SM8150     = CHIPINFO_FAMILY_SDM855,
	CHIPINFO_FAMILY_SM8250     = CHIPINFO_FAMILY_SDA865,
	CHIPINFO_FAMILY_SCX8180    = CHIPINFO_FAMILY_SDM1000,
	CHIPINFO_FAMILY_SM7150     = 96,
	CHIPINFO_FAMILY_SM6150     = CHIPINFO_FAMILY_SDM640,
	CHIPINFO_FAMILY_SXR1130    = 97,
	CHIPINFO_FAMILY_QCN7605    = 98,
	CHIPINFO_FAMILY_SC8180X    = CHIPINFO_FAMILY_SCX8180,
	CHIPINFO_FAMILY_MDM9205    = 99,
	CHIPINFO_FAMILY_NICOBAR    = 100,
	CHIPINFO_FAMILY_SAIPAN     = 101,
	CHIPINFO_FAMILY_IPQ60xx    = 102,
	CHIPINFO_FAMILY_RENNELL    = 103,
	CHIPINFO_FAMILY_QCA6490    = 104,
	CHIPINFO_FAMILY_LAHAINA    = 105,
	CHIPINFO_FAMILY_KAMORTA    = 106,
	CHIPINFO_FAMILY_QCN90xx    = 107,
	CHIPINFO_FAMILY_BITRA      = 108,
	CHIPINFO_FAMILY_CHITWAN    = 109,
	CHIPINFO_FAMILY_AGATTI     = 110,
	CHIPINFO_FAMILY_MOSELLE    = 111,
	CHIPINFO_FAMILY_IPQ50xx    = 112,
	CHIPINFO_FAMILY_MAKENA     = 113,
	CHIPINFO_FAMILY_CEDROS     = 114,
	CHIPINFO_FAMILY_MANNAR     = 115,
	CHIPINFO_FAMILY_WAIPIO     = 116,
	CHIPINFO_FAMILY_OLYMPIC    = 117,
	CHIPINFO_FAMILY_KODIAK     = 118,
	CHIPINFO_FAMILY_QCN91xx    = 119,
	CHIPINFO_FAMILY_WCN7850    = 120,
	CHIPINFO_FAMILY_ATHERTON   = 121,
	CHIPINFO_FAMILY_QTANG2     = 122,
	CHIPINFO_FAMILY_FILLMORE   = 123,
	CHIPINFO_FAMILY_STRAIT     = 124,
	CHIPINFO_FAMILY_IPQ90xx    = 125,
	CHIPINFO_FAMILY_DIVAR      = 126,
	CHIPINFO_FAMILY_KAILUA     = 127,
	CHIPINFO_NUM_FAMILIES,
	CHIPINFO_FAMILY_32BITS     = 0x7FFFFFF
}ChipInfoFamilyType;
typedef UINT32 ChipInfoSerialNumType;
typedef UINT32 ChipInfoQFPROMChipIdType;
typedef enum{
	CHIPINFO_FOUNDRYID_UNKNOWN = 0,
	CHIPINFO_FOUNDRYID_TSMC    = 1,
	CHIPINFO_FOUNDRYID_GF      = 2,
	CHIPINFO_FOUNDRYID_SS      = 3,
	CHIPINFO_FOUNDRYID_IBM     = 4,
	CHIPINFO_FOUNDRYID_UMC     = 5,
	CHIPINFO_FOUNDRYID_SMIC    = 6,
	CHIPINFO_NUM_FOUNDRYIDS,
	CHIPINFO_FOUNDRYID_32BITS  = 0x7FFFFFF
}ChipInfoFoundryIdType;
typedef enum{
	CHIPINFO_PART_UNKNOWN      = 0,
	CHIPINFO_PART_GPU          = 1,
	CHIPINFO_PART_VIDEO        = 2,
	CHIPINFO_PART_CAMERA       = 3,
	CHIPINFO_PART_DISPLAY      = 4,
	CHIPINFO_PART_AUDIO        = 5,
	CHIPINFO_PART_MODEM        = 6,
	CHIPINFO_PART_WLAN         = 7,
	CHIPINFO_PART_COMP         = 8,
	CHIPINFO_PART_SENSORS      = 9,
	CHIPINFO_PART_NPU          = 10,
	CHIPINFO_PART_SPSS         = 11,
	CHIPINFO_PART_NAV          = 12,
	CHIPINFO_NUM_PARTS,
	CHIPINFO_PART_32BITS = 0x7FFFFFFF
}ChipInfoPartType;
typedef struct _CHIPINFO_PROTOCOL CHIPINFO_PROTOCOL;
extern EFI_GUID gEfiChipInfoProtocolGuid;
typedef UINT32 ChipInfoVersionType;
typedef UINT32 ChipInfoModemType;
typedef UINT32 ChipInfoSerialNumType;
typedef UINT32 ChipInfoQFPROMChipIdType;
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETCHIPVERSION)(
	IN  CHIPINFO_PROTOCOL   *This,
	OUT ChipInfoVersionType *pnVersion
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETRAWCHIPVERSION)(
	IN  CHIPINFO_PROTOCOL *This,
	OUT UINT32            *pnVersion
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETCHIPID)(
	IN  CHIPINFO_PROTOCOL *This,
	OUT ChipInfoIdType    *peId
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETRAWCHIPID)(
	IN  CHIPINFO_PROTOCOL *This,
	OUT UINT32            *pnId
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETCHIPIDSTRING)(
	IN  CHIPINFO_PROTOCOL *This,
	OUT CHAR8             *szIdString,
	IN  UINT32            nMaxLength
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETCHIPFAMILY)(
	IN  CHIPINFO_PROTOCOL  *This,
	OUT ChipInfoFamilyType *peFamily
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETMODEMSUPPORT)(
	IN  CHIPINFO_PROTOCOL *This,
	OUT ChipInfoModemType *pnModem
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETPROCESSORNAMESTRING)(
	IN  CHIPINFO_PROTOCOL *This,
	OUT CHAR8             *szNameString,
	IN  UINT32            nMaxLength
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETSERIALNUMBER)(
	IN  CHIPINFO_PROTOCOL     *This,
	OUT ChipInfoSerialNumType *peId
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETFOUNDRYID)(
	IN  CHIPINFO_PROTOCOL     *This,
	OUT ChipInfoFoundryIdType *peId
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETRAWDEVICEFAMILY) (
	IN  CHIPINFO_PROTOCOL *This,
	OUT UINT32            *pnId
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETRAWDEVICENUMBER)(
	IN  CHIPINFO_PROTOCOL *This,
	OUT UINT32            *pnId
);
typedef EFI_STATUS (EFIAPI *CHIPINFO_GETQFPROMCHIPID)(
	IN  CHIPINFO_PROTOCOL        *This,
	OUT ChipInfoQFPROMChipIdType *pnId
);
typedef EFI_STATUS (EFIAPI*CHIPINFO_GETMARKETINGNAMESTRING)(
	IN  CHIPINFO_PROTOCOL *This,
	OUT CHAR8             *szNameString,
	IN  UINT32            nMaxLength
);
typedef EFI_STATUS(EFIAPI*CHIPINFO_GETDEFECTIVEPART)(
	IN  CHIPINFO_PROTOCOL *This,
	IN  ChipInfoPartType  ePart,
	OUT UINT32            *pnMask
);
typedef EFI_STATUS (EFIAPI*CHIPINFO_GETDEFECTIVECPUS)(
	IN  CHIPINFO_PROTOCOL *This,
	IN  UINT32            nCPUCluster,
	OUT UINT32            *pnMask
);
struct _CHIPINFO_PROTOCOL {
	UINT64 Revision;
	CHIPINFO_GETCHIPVERSION         GetChipVersion;
	CHIPINFO_GETRAWCHIPVERSION      GetRawChipVersion;
	CHIPINFO_GETCHIPID              GetChipId;
	CHIPINFO_GETRAWCHIPID           GetRawChipId;
	CHIPINFO_GETCHIPIDSTRING        GetChipIdString;
	CHIPINFO_GETCHIPFAMILY          GetChipFamily;
	CHIPINFO_GETMODEMSUPPORT        GetModemSupport;
	CHIPINFO_GETPROCESSORNAMESTRING GetProcessorNameString;
	CHIPINFO_GETSERIALNUMBER        GetSerialNumber;
	CHIPINFO_GETFOUNDRYID           GetFoundryId;
	CHIPINFO_GETRAWDEVICEFAMILY     GetRawDeviceFamily;
	CHIPINFO_GETRAWDEVICENUMBER     GetRawDeviceNumber;
	CHIPINFO_GETQFPROMCHIPID        GetQFPROMChipId;
	CHIPINFO_GETMARKETINGNAMESTRING GetMarketingNameString;
	CHIPINFO_GETDEFECTIVEPART       GetDefectivePart;
	CHIPINFO_GETDEFECTIVECPUS       GetDefectiveCPUs;
};
#endif
