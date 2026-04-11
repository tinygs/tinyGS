#ifndef TINYGS_HTML_H
#define TINYGS_HTML_H
/*
  htmlOptions.h - Static html to print the options on the cofig form
  
  Copyright (C) 2020 -2021 @G4lile0, @gmag12 and @dev_4m1g0

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

constexpr auto TZ_LENGTH = 49;
const char TZ_VALUES[][TZ_LENGTH] PROGMEM = {"000GMT0", "001GMT0", "002EAT-3", "003CET-1", "004EAT-3", "005GMT0", "006WAT-1", "007GMT0", "008GMT0", "009CAT-2", "010WAT-1", "011CAT-2", "012EET-2", "013<+01>-1", "014CET-1CEST,M3.5.0,M10.5.0/3", "015GMT0", "016GMT0", "017EAT-3", "018EAT-3", "019WAT-1", "020<+01>-1", "021GMT0", "022CAT-2", "023CAT-2", "024SAST-2", "025EAT-3", "026EAT-3", "027CAT-2", "028CAT-2", "029WAT-1", "030WAT-1", "031WAT-1", "032GMT0", "033WAT-1", "034CAT-2", "035CAT-2", "036WAT-1", "037CAT-2", "038SAST-2", "039SAST-2", "040EAT-3", "041GMT0", "042EAT-3", "043WAT-1", "044WAT-1", "045GMT0", "046GMT0", "047WAT-1", "048GMT0", "049EET-2", "050CET-1", "051CAT-2", "052HST10HDT,M3.2.0,M11.1.0", "053AKST9AKDT,M3.2.0,M11.1.0", "054AST4", "055AST4", "056<-03>3", "057<-03>3", "058<-03>3", "059<-03>3", "060<-03>3", "061<-03>3", "062<-03>3", "063<-03>3", "064<-03>3", "065<-03>3", "066<-03>3", "067<-03>3", "068<-03>3", "069AST4", "070<-04>4<-03>,M10.1.0/0,M3.4.0/0", "071EST5", "072<-03>3", "073CST6CDT,M4.1.0,M10.5.0", "074AST4", "075<-03>3", "076CST6", "077AST4", "078<-04>4", "079<-05>5", "080MST7MDT,M3.2.0,M11.1.0", "081MST7MDT,M3.2.0,M11.1.0", "082<-04>4", "083EST5", "084<-04>4", "085<-03>3", "086EST5", "087CST6CDT,M3.2.0,M11.1.0", "088MST7MDT,M4.1.0,M10.5.0", "089CST6", "090MST7", "091<-04>4", "092AST4", "093GMT0", "094PST8PDT,M3.2.0,M11.1.0", "095MST7", "096MST7MDT,M3.2.0,M11.1.0", "097EST5EDT,M3.2.0,M11.1.0", "098AST4", "099MST7MDT,M3.2.0,M11.1.0", "100<-05>5", "101CST6", "102<-03>3", "103MST7", "104AST4ADT,M3.2.0,M11.1.0", "105<-03>3<-02>,M3.5.0/-2,M10.5.0/-1", "106AST4ADT,M3.2.0,M11.1.0", "107EST5EDT,M3.2.0,M11.1.0", "108AST4", "109AST4", "110CST6", "111<-05>5", "112<-04>4", "113AST4ADT,M3.2.0,M11.1.0", "114CST5CDT,M3.2.0/0,M11.1.0/1", "115MST7", "116EST5EDT,M3.2.0,M11.1.0", "117CST6CDT,M3.2.0,M11.1.0", "118EST5EDT,M3.2.0,M11.1.0", "119EST5EDT,M3.2.0,M11.1.0", "120CST6CDT,M3.2.0,M11.1.0", "121EST5EDT,M3.2.0,M11.1.0", "122EST5EDT,M3.2.0,M11.1.0", "123EST5EDT,M3.2.0,M11.1.0", "124MST7MDT,M3.2.0,M11.1.0", "125EST5EDT,M3.2.0,M11.1.0", "126EST5", "127AKST9AKDT,M3.2.0,M11.1.0", "128EST5EDT,M3.2.0,M11.1.0", "129EST5EDT,M3.2.0,M11.1.0", "130AST4", "131<-04>4", "132<-05>5", "133PST8PDT,M3.2.0,M11.1.0", "134AST4", "135<-03>3", "136CST6", "137<-04>4", "138AST4", "139AST4", "140CST6CDT,M3.2.0,M11.1.0", "141MST7MDT,M4.1.0,M10.5.0", "142CST6CDT,M3.2.0,M11.1.0", "143CST6CDT,M4.1.0,M10.5.0", "144AKST9AKDT,M3.2.0,M11.1.0", "145CST6CDT,M4.1.0,M10.5.0", "146<-03>3<-02>,M3.2.0,M11.1.0", "147AST4ADT,M3.2.0,M11.1.0", "148CST6CDT,M4.1.0,M10.5.0", "149<-03>3", "150EST5EDT,M3.2.0,M11.1.0", "151AST4", "152EST5EDT,M3.2.0,M11.1.0", "153EST5EDT,M3.2.0,M11.1.0", "154EST5EDT,M3.2.0,M11.1.0", "155AKST9AKDT,M3.2.0,M11.1.0", "156<-02>2", "157CST6CDT,M3.2.0,M11.1.0", "158CST6CDT,M3.2.0,M11.1.0", "159CST6CDT,M3.2.0,M11.1.0", "160MST7MDT,M3.2.0,M11.1.0", "161EST5", "162EST5EDT,M3.2.0,M11.1.0", "163<-03>3", "164MST7", "165EST5EDT,M3.2.0,M11.1.0", "166AST4", "167<-04>4", "168AST4", "169<-03>3", "170CST6CDT,M3.2.0,M11.1.0", "171CST6CDT,M3.2.0,M11.1.0", "172<-03>3", "173CST6", "174CST6CDT,M3.2.0,M11.1.0", "175<-05>5", "176<-03>3", "177<-04>4<-03>,M9.1.6/24,M4.1.6/24", "178AST4", "179<-03>3", "180<-01>1<+00>,M3.5.0/0,M10.5.0/1", "181AKST9AKDT,M3.2.0,M11.1.0", "182AST4", "183NST3:30NDT,M3.2.0,M11.1.0", "184AST4", "185AST4", "186AST4", "187AST4", "188CST6", "189CST6", "190AST4ADT,M3.2.0,M11.1.0", "191EST5EDT,M3.2.0,M11.1.0", "192PST8PDT,M3.2.0,M11.1.0", "193EST5EDT,M3.2.0,M11.1.0", "194AST4", "195PST8PDT,M3.2.0,M11.1.0", "196PST8PDT,M3.2.0,M11.1.0", "197CST6CDT,M3.2.0,M11.1.0", "198AKST9AKDT,M3.2.0,M11.1.0", "199MST7MDT,M3.2.0,M11.1.0", "200<+08>-8", "201<+07>-7", "202<+10>-10", "203<+11>-11", "204<+05>-5", "205NZST-12NZDT,M9.5.0,M4.1.0/3", "206<-03>3", "207<-03>3", "208<+03>-3", "209<+00>0<+02>-2,M3.5.0/1,M10.5.0/3", "210<+06>-6", "211CET-1CEST,M3.5.0,M10.5.0/3", "212<+03>-3", "213<+06>-6", "214EET-2EEST,M3.5.4/24,M10.5.5/1", "215<+12>-12", "216<+05>-5", "217<+05>-5", "218<+05>-5", "219<+05>-5", "220<+03>-3", "221<+03>-3", "222<+04>-4", "223<+07>-7", "224<+07>-7", "225EET-2EEST,M3.5.0/0,M10.5.0/0", "226<+06>-6", "227<+08>-8", "228<+09>-9", "229<+08>-8", "230<+0530>-5:30", "231EET-2EEST,M3.5.5/0,M10.5.5/0", "232<+06>-6", "233<+09>-9", "234<+04>-4", "235<+05>-5", "236EET-2EEST,M3.5.0/3,M10.5.0/4", "237EET-2EEST,M3.5.5/0,M10.5.6/1", "238EET-2EEST,M3.5.5/0,M10.5.6/1", "239<+07>-7", "240HKT-8", "241<+07>-7", "242<+08>-8", "243WIB-7", "244WIT-9", "245IST-2IDT,M3.4.4/26,M10.5.0", "246<+0430>-4:30", "247<+12>-12", "248PKT-5", "249<+0545>-5:45", "250<+09>-9", "251IST-5:30", "252<+07>-7", "253<+08>-8", "254<+08>-8", "255<+03>-3", "256CST-8", "257<+11>-11", "258WITA-8", "259PST-8", "260<+04>-4", "261EET-2EEST,M3.5.0/3,M10.5.0/4", "262<+07>-7", "263<+07>-7", "264<+06>-6", "265<+05>-5", "266<+07>-7", "267WIB-7", "268KST-9", "269<+03>-3", "270<+05>-5", "271<+03>-3", "272<+11>-11", "273<+05>-5", "274KST-9", "275CST-8", "276<+08>-8", "277<+11>-11", "278CST-8", "279<+05>-5", "280<+04>-4", "281<+0330>-3:30<+0430>,J79/24,J263/24", "282<+06>-6", "283JST-9", "284<+07>-7", "285<+08>-8", "286<+06>-6", "287<+10>-10", "288<+07>-7", "289<+10>-10", "290<+09>-9", "291<+0630>-6:30", "292<+05>-5", "293<+04>-4", "294<-01>1<+00>,M3.5.0/0,M10.5.0/1", "295AST4ADT,M3.2.0,M11.1.0", "296WET0WEST,M3.5.0/1,M10.5.0", "297<-01>1", "298WET0WEST,M3.5.0/1,M10.5.0", "299WET0WEST,M3.5.0/1,M10.5.0", "300GMT0", "301<-02>2", "302<-03>3", "303GMT0", "304ACST-9:30ACDT,M10.1.0,M4.1.0/3", "305AEST-10", "306ACST-9:30ACDT,M10.1.0,M4.1.0/3", "307AEST-10AEDT,M10.1.0,M4.1.0/3", "308ACST-9:30", "309<+0845>-8:45", "310AEST-10AEDT,M10.1.0,M4.1.0/3", "311AEST-10", "312<+1030>-10:30<+11>-11,M10.1.0,M4.1.0", "313AEST-10AEDT,M10.1.0,M4.1.0/3", "314AWST-8", "315AEST-10AEDT,M10.1.0,M4.1.0/3", "316CET-1CEST,M3.5.0,M10.5.0/3", "317CET-1CEST,M3.5.0,M10.5.0/3", "318<+04>-4", "319EET-2EEST,M3.5.0/3,M10.5.0/4", "320CET-1CEST,M3.5.0,M10.5.0/3", "321CET-1CEST,M3.5.0,M10.5.0/3", "322CET-1CEST,M3.5.0,M10.5.0/3", "323CET-1CEST,M3.5.0,M10.5.0/3", "324EET-2EEST,M3.5.0/3,M10.5.0/4", "325CET-1CEST,M3.5.0,M10.5.0/3", "326CET-1CEST,M3.5.0,M10.5.0/3", "327EET-2EEST,M3.5.0,M10.5.0/3", "328CET-1CEST,M3.5.0,M10.5.0/3", "329IST-1GMT0,M10.5.0,M3.5.0/1", "330CET-1CEST,M3.5.0,M10.5.0/3", "331GMT0BST,M3.5.0/1,M10.5.0", "332EET-2EEST,M3.5.0/3,M10.5.0/4", "333GMT0BST,M3.5.0/1,M10.5.0", "334<+03>-3", "335GMT0BST,M3.5.0/1,M10.5.0", "336EET-2", "337EET-2EEST,M3.5.0/3,M10.5.0/4", "338<+03>-3", "339WET0WEST,M3.5.0/1,M10.5.0", "340CET-1CEST,M3.5.0,M10.5.0/3", "341GMT0BST,M3.5.0/1,M10.5.0", "342CET-1CEST,M3.5.0,M10.5.0/3", "343CET-1CEST,M3.5.0,M10.5.0/3", "344CET-1CEST,M3.5.0,M10.5.0/3", "345EET-2EEST,M3.5.0/3,M10.5.0/4", "346<+03>-3", "347CET-1CEST,M3.5.0,M10.5.0/3", "348MSK-3", "349CET-1CEST,M3.5.0,M10.5.0/3", "350CET-1CEST,M3.5.0,M10.5.0/3", "351CET-1CEST,M3.5.0,M10.5.0/3", "352CET-1CEST,M3.5.0,M10.5.0/3", "353EET-2EEST,M3.5.0/3,M10.5.0/4", "354CET-1CEST,M3.5.0,M10.5.0/3", "355<+04>-4", "356CET-1CEST,M3.5.0,M10.5.0/3", "357CET-1CEST,M3.5.0,M10.5.0/3", "358<+04>-4", "359MSK-3", "360CET-1CEST,M3.5.0,M10.5.0/3", "361EET-2EEST,M3.5.0/3,M10.5.0/4", "362CET-1CEST,M3.5.0,M10.5.0/3", "363EET-2EEST,M3.5.0/3,M10.5.0/4", "364CET-1CEST,M3.5.0,M10.5.0/3", "365<+04>-4", "366EET-2EEST,M3.5.0/3,M10.5.0/4", "367CET-1CEST,M3.5.0,M10.5.0/3", "368CET-1CEST,M3.5.0,M10.5.0/3", "369CET-1CEST,M3.5.0,M10.5.0/3", "370EET-2EEST,M3.5.0/3,M10.5.0/4", "371<+04>-4", "372CET-1CEST,M3.5.0,M10.5.0/3", "373CET-1CEST,M3.5.0,M10.5.0/3", "374EET-2EEST,M3.5.0/3,M10.5.0/4", "375CET-1CEST,M3.5.0,M10.5.0/3", "376EAT-3", "377<+06>-6", "378<+07>-7", "379<+0630>-6:30", "380EAT-3", "381<+05>-5", "382<+04>-4", "383<+05>-5", "384<+04>-4", "385EAT-3", "386<+04>-4", "387<+13>-13<+14>,M9.5.0/3,M4.1.0/4", "388NZST-12NZDT,M9.5.0,M4.1.0/3", "389<+11>-11", "390<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45", "391<+10>-10", "392<-06>6<-05>,M9.1.6/22,M4.1.6/22", "393<+11>-11", "394<+13>-13", "395<+13>-13", "396<+12>-12<+13>,M11.2.0,M1.2.3/99", "397<+12>-12", "398<-06>6", "399<-09>9", "400<+11>-11", "401ChST-10", "402HST10", "403<+14>-14", "404<+11>-11", "405<+12>-12", "406<+12>-12", "407<-0930>9:30", "408SST11", "409<+12>-12", "410<-11>11", "411<+11>-11<+12>,M10.1.0,M4.1.0/3", "412<+11>-11", "413SST11", "414<+09>-9", "415<-08>8", "416<+11>-11", "417<+10>-10", "418<-10>10", "419ChST-10", "420<-10>10", "421<+12>-12", "422<+13>-13", "423<+12>-12", "424<+12>-12", "425GMT0", "426GMT0", "427<+01>-1", "428<+02>-2", "429<+03>-3", "430<+04>-4", "431<+05>-5", "432<+06>-6", "433<+07>-7", "434<+08>-8", "435<+09>-9", "436<+10>-10", "437<+11>-11", "438<+12>-12", "439<+13>-13", "440<+14>-14", "441GMT0", "442GMT0", "443<-01>1", "444<-02>2", "445<-03>3", "446<-04>4", "447<-05>5", "448<-06>6", "449<-07>7", "450<-08>8", "451<-09>9", "452<-10>10", "453<-11>11", "454<-12>12", "455UTC0", "456UTC0", "457GMT0", "458UTC0", "459UTC0"};

constexpr auto TZ_NAME_LENGTH = 32;
const char TZ_NAMES[][TZ_NAME_LENGTH] PROGMEM = {"Africa/Abidjan", "Africa/Accra", "Africa/Addis_Ababa", "Africa/Algiers", "Africa/Asmara", "Africa/Bamako", "Africa/Bangui", "Africa/Banjul", "Africa/Bissau", "Africa/Blantyre", "Africa/Brazzaville", "Africa/Bujumbura", "Africa/Cairo", "Africa/Casablanca", "Africa/Ceuta", "Africa/Conakry", "Africa/Dakar", "Africa/Dar_es_Salaam", "Africa/Djibouti", "Africa/Douala", "Africa/El_Aaiun", "Africa/Freetown", "Africa/Gaborone", "Africa/Harare", "Africa/Johannesburg", "Africa/Juba", "Africa/Kampala", "Africa/Khartoum", "Africa/Kigali", "Africa/Kinshasa", "Africa/Lagos", "Africa/Libreville", "Africa/Lome", "Africa/Luanda", "Africa/Lubumbashi", "Africa/Lusaka", "Africa/Malabo", "Africa/Maputo", "Africa/Maseru", "Africa/Mbabane", "Africa/Mogadishu", "Africa/Monrovia", "Africa/Nairobi", "Africa/Ndjamena", "Africa/Niamey", "Africa/Nouakchott", "Africa/Ouagadougou", "Africa/Porto-Novo", "Africa/Sao_Tome", "Africa/Tripoli", "Africa/Tunis", "Africa/Windhoek", "America/Adak", "America/Anchorage", "America/Anguilla", "America/Antigua", "America/Araguaina", "America/Argentina/Buenos_Aires", "America/Argentina/Catamarca", "America/Argentina/Cordoba", "America/Argentina/Jujuy", "America/Argentina/La_Rioja", "America/Argentina/Mendoza", "America/Argentina/Rio_Gallegos", "America/Argentina/Salta", "America/Argentina/San_Juan", "America/Argentina/San_Luis", "America/Argentina/Tucuman", "America/Argentina/Ushuaia", "America/Aruba", "America/Asuncion", "America/Atikokan", "America/Bahia", "America/Bahia_Banderas", "America/Barbados", "America/Belem", "America/Belize", "America/Blanc-Sablon", "America/Boa_Vista", "America/Bogota", "America/Boise", "America/Cambridge_Bay", "America/Campo_Grande", "America/Cancun", "America/Caracas", "America/Cayenne", "America/Cayman", "America/Chicago", "America/Chihuahua", "America/Costa_Rica", "America/Creston", "America/Cuiaba", "America/Curacao", "America/Danmarkshavn", "America/Dawson", "America/Dawson_Creek", "America/Denver", "America/Detroit", "America/Dominica", "America/Edmonton", "America/Eirunepe", "America/El_Salvador", "America/Fortaleza", "America/Fort_Nelson", "America/Glace_Bay", "America/Godthab", "America/Goose_Bay", "America/Grand_Turk", "America/Grenada", "America/Guadeloupe", "America/Guatemala", "America/Guayaquil", "America/Guyana", "America/Halifax", "America/Havana", "America/Hermosillo", "America/Indiana/Indianapolis", "America/Indiana/Knox", "America/Indiana/Marengo", "America/Indiana/Petersburg", "America/Indiana/Tell_City", "America/Indiana/Vevay", "America/Indiana/Vincennes", "America/Indiana/Winamac", "America/Inuvik", "America/Iqaluit", "America/Jamaica", "America/Juneau", "America/Kentucky/Louisville", "America/Kentucky/Monticello", "America/Kralendijk", "America/La_Paz", "America/Lima", "America/Los_Angeles", "America/Lower_Princes", "America/Maceio", "America/Managua", "America/Manaus", "America/Marigot", "America/Martinique", "America/Matamoros", "America/Mazatlan", "America/Menominee", "America/Merida", "America/Metlakatla", "America/Mexico_City", "America/Miquelon", "America/Moncton", "America/Monterrey", "America/Montevideo", "America/Montreal", "America/Montserrat", "America/Nassau", "America/New_York", "America/Nipigon", "America/Nome", "America/Noronha", "America/North_Dakota/Beulah", "America/North_Dakota/Center", "America/North_Dakota/New_Salem", "America/Ojinaga", "America/Panama", "America/Pangnirtung", "America/Paramaribo", "America/Phoenix", "America/Port-au-Prince", "America/Port_of_Spain", "America/Porto_Velho", "America/Puerto_Rico", "America/Punta_Arenas", "America/Rainy_River", "America/Rankin_Inlet", "America/Recife", "America/Regina", "America/Resolute", "America/Rio_Branco", "America/Santarem", "America/Santiago", "America/Santo_Domingo", "America/Sao_Paulo", "America/Scoresbysund", "America/Sitka", "America/St_Barthelemy", "America/St_Johns", "America/St_Kitts", "America/St_Lucia", "America/St_Thomas", "America/St_Vincent", "America/Swift_Current", "America/Tegucigalpa", "America/Thule", "America/Thunder_Bay", "America/Tijuana", "America/Toronto", "America/Tortola", "America/Vancouver", "America/Whitehorse", "America/Winnipeg", "America/Yakutat", "America/Yellowknife", "Antarctica/Casey", "Antarctica/Davis", "Antarctica/DumontDUrville", "Antarctica/Macquarie", "Antarctica/Mawson", "Antarctica/McMurdo", "Antarctica/Palmer", "Antarctica/Rothera", "Antarctica/Syowa", "Antarctica/Troll", "Antarctica/Vostok", "Arctic/Longyearbyen", "Asia/Aden", "Asia/Almaty", "Asia/Amman", "Asia/Anadyr", "Asia/Aqtau", "Asia/Aqtobe", "Asia/Ashgabat", "Asia/Atyrau", "Asia/Baghdad", "Asia/Bahrain", "Asia/Baku", "Asia/Bangkok", "Asia/Barnaul", "Asia/Beirut", "Asia/Bishkek", "Asia/Brunei", "Asia/Chita", "Asia/Choibalsan", "Asia/Colombo", "Asia/Damascus", "Asia/Dhaka", "Asia/Dili", "Asia/Dubai", "Asia/Dushanbe", "Asia/Famagusta", "Asia/Gaza", "Asia/Hebron", "Asia/Ho_Chi_Minh", "Asia/Hong_Kong", "Asia/Hovd", "Asia/Irkutsk", "Asia/Jakarta", "Asia/Jayapura", "Asia/Jerusalem", "Asia/Kabul", "Asia/Kamchatka", "Asia/Karachi", "Asia/Kathmandu", "Asia/Khandyga", "Asia/Kolkata", "Asia/Krasnoyarsk", "Asia/Kuala_Lumpur", "Asia/Kuching", "Asia/Kuwait", "Asia/Macau", "Asia/Magadan", "Asia/Makassar", "Asia/Manila", "Asia/Muscat", "Asia/Nicosia", "Asia/Novokuznetsk", "Asia/Novosibirsk", "Asia/Omsk", "Asia/Oral", "Asia/Phnom_Penh", "Asia/Pontianak", "Asia/Pyongyang", "Asia/Qatar", "Asia/Qyzylorda", "Asia/Riyadh", "Asia/Sakhalin", "Asia/Samarkand", "Asia/Seoul", "Asia/Shanghai", "Asia/Singapore", "Asia/Srednekolymsk", "Asia/Taipei", "Asia/Tashkent", "Asia/Tbilisi", "Asia/Tehran", "Asia/Thimphu", "Asia/Tokyo", "Asia/Tomsk", "Asia/Ulaanbaatar", "Asia/Urumqi", "Asia/Ust-Nera", "Asia/Vientiane", "Asia/Vladivostok", "Asia/Yakutsk", "Asia/Yangon", "Asia/Yekaterinburg", "Asia/Yerevan", "Atlantic/Azores", "Atlantic/Bermuda", "Atlantic/Canary", "Atlantic/Cape_Verde", "Atlantic/Faroe", "Atlantic/Madeira", "Atlantic/Reykjavik", "Atlantic/South_Georgia", "Atlantic/Stanley", "Atlantic/St_Helena", "Australia/Adelaide", "Australia/Brisbane", "Australia/Broken_Hill", "Australia/Currie", "Australia/Darwin", "Australia/Eucla", "Australia/Hobart", "Australia/Lindeman", "Australia/Lord_Howe", "Australia/Melbourne", "Australia/Perth", "Australia/Sydney", "Europe/Amsterdam", "Europe/Andorra", "Europe/Astrakhan", "Europe/Athens", "Europe/Belgrade", "Europe/Berlin", "Europe/Bratislava", "Europe/Brussels", "Europe/Bucharest", "Europe/Budapest", "Europe/Busingen", "Europe/Chisinau", "Europe/Copenhagen", "Europe/Dublin", "Europe/Gibraltar", "Europe/Guernsey", "Europe/Helsinki", "Europe/Isle_of_Man", "Europe/Istanbul", "Europe/Jersey", "Europe/Kaliningrad", "Europe/Kiev", "Europe/Kirov", "Europe/Lisbon", "Europe/Ljubljana", "Europe/London", "Europe/Luxembourg", "Europe/Madrid", "Europe/Malta", "Europe/Mariehamn", "Europe/Minsk", "Europe/Monaco", "Europe/Moscow", "Europe/Oslo", "Europe/Paris", "Europe/Podgorica", "Europe/Prague", "Europe/Riga", "Europe/Rome", "Europe/Samara", "Europe/San_Marino", "Europe/Sarajevo", "Europe/Saratov", "Europe/Simferopol", "Europe/Skopje", "Europe/Sofia", "Europe/Stockholm", "Europe/Tallinn", "Europe/Tirane", "Europe/Ulyanovsk", "Europe/Uzhgorod", "Europe/Vaduz", "Europe/Vatican", "Europe/Vienna", "Europe/Vilnius", "Europe/Volgograd", "Europe/Warsaw", "Europe/Zagreb", "Europe/Zaporozhye", "Europe/Zurich", "Indian/Antananarivo", "Indian/Chagos", "Indian/Christmas", "Indian/Cocos", "Indian/Comoro", "Indian/Kerguelen", "Indian/Mahe", "Indian/Maldives", "Indian/Mauritius", "Indian/Mayotte", "Indian/Reunion", "Pacific/Apia", "Pacific/Auckland", "Pacific/Bougainville", "Pacific/Chatham", "Pacific/Chuuk", "Pacific/Easter", "Pacific/Efate", "Pacific/Enderbury", "Pacific/Fakaofo", "Pacific/Fiji", "Pacific/Funafuti", "Pacific/Galapagos", "Pacific/Gambier", "Pacific/Guadalcanal", "Pacific/Guam", "Pacific/Honolulu", "Pacific/Kiritimati", "Pacific/Kosrae", "Pacific/Kwajalein", "Pacific/Majuro", "Pacific/Marquesas", "Pacific/Midway", "Pacific/Nauru", "Pacific/Niue", "Pacific/Norfolk", "Pacific/Noumea", "Pacific/Pago_Pago", "Pacific/Palau", "Pacific/Pitcairn", "Pacific/Pohnpei", "Pacific/Port_Moresby", "Pacific/Rarotonga", "Pacific/Saipan", "Pacific/Tahiti", "Pacific/Tarawa", "Pacific/Tongatapu", "Pacific/Wake", "Pacific/Wallis", "Etc/GMT", "Etc/GMT-0", "Etc/GMT-1", "Etc/GMT-2", "Etc/GMT-3", "Etc/GMT-4", "Etc/GMT-5", "Etc/GMT-6", "Etc/GMT-7", "Etc/GMT-8", "Etc/GMT-9", "Etc/GMT-10", "Etc/GMT-11", "Etc/GMT-12", "Etc/GMT-13", "Etc/GMT-14", "Etc/GMT0", "Etc/GMT+0", "Etc/GMT+1", "Etc/GMT+2", "Etc/GMT+3", "Etc/GMT+4", "Etc/GMT+5", "Etc/GMT+6", "Etc/GMT+7", "Etc/GMT+8", "Etc/GMT+9", "Etc/GMT+10", "Etc/GMT+11", "Etc/GMT+12", "Etc/UCT", "Etc/UTC", "Etc/Greenwich", "Etc/Universal", "Etc/Zulu"};

constexpr auto BOARD_LENGTH = 3;  // max chars for board index string (e.g. "23")

#if CONFIG_IDF_TARGET_ESP32S3
const char BOARD_VALUES[][BOARD_LENGTH] PROGMEM = {"0", "1", "2", "3", "4", "5", "6", "7"};
#elif CONFIG_IDF_TARGET_ESP32C3
const char BOARD_VALUES[][BOARD_LENGTH] PROGMEM = {"0", "1"  };
#else 
const char BOARD_VALUES[][BOARD_LENGTH] PROGMEM = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23"};
#endif

const char IOTWEBCONF_DASHBOARD_STYLE_INNER[] PROGMEM =
    ".dash-wrap{max-width:1440px;margin:0 auto;padding:0.5rem;}"
    ".cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:0.75rem;margin:0.75rem 0;}"
    ".card{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);padding:1rem;text-align:left;transition:border-color .25s;}"
    ".card:hover{border-color:var(--accent);}"
    ".card h3{font-size:0.85rem;color:var(--accent2);text-transform:uppercase;letter-spacing:0.08em;margin:0 0 0.6rem;padding-bottom:0.4rem;border-bottom:1px solid var(--border);text-align:left;}"
    ".card table{width:100%;border-collapse:collapse;}"
    ".card td{padding:0.25rem 0.4rem;font-size:0.82rem;border:none;}"
    ".card td:first-child{color:var(--text2);white-space:nowrap;width:45%;}"
    ".card td:last-child{color:var(--text);font-weight:500;}"
    ".map-wrap{background:var(--surface);border:1px solid var(--border);border-radius:var(--radius);padding:0.75rem;margin:0.75rem auto;max-width:460px;}"
    ".map-wrap svg{width:100%;height:auto;}"
    "textarea{resize:vertical;width:100%;height:300px;background:var(--surface2);color:var(--text);border:1px solid var(--border);border-radius:var(--radius);padding:0.6rem;font-family:'Courier New',monospace;font-size:0.78rem;line-height:1.5;}"
    "html.dark textarea{background:#0d1117;color:#22c55e;}"
    "#c1{width:100%;margin-top:0.4rem;font-family:'Courier New',monospace;font-size:0.85rem;}"
    ".console{display:block;max-width:1440px;margin:0.75rem auto;text-align:center;}";
const char IOTWEBCONF_DASHBOARD_BODY_INNER[] PROGMEM = "<div class='dash-wrap'>\n";
const char IOTWEBCONF_CONSOLE_BODY_INNER[] PROGMEM = "<div class='console'><textarea readonly='' id='t1' wrap='off' name='t1'></textarea><form method='get' onsubmit='return f(1);'><input id='c1' placeholder='> Enter command...' autofocus='' name='c1'></form></div>\n";
const char IOTWEBCONF_CONSOLE_SCRIPT[] PROGMEM  = "var x=null,lt,to,tp,pc='';var sn=0,id=0;function f(p){var c,o='',t;clearTimeout(lt);t = document.getElementById('t1');if (p==1) {c =document.getElementById('c1');o='&c1='+encodeURIComponent(c.value);c.value='';t.scrollTop=99999;sn=t.scrollTop;}if (t.scrollTop >= sn){if (x!=null){x.abort();}x=new XMLHttpRequest();x.onreadystatechange=function() {if(x.readyState==4&&wmx.status==200){var z,d;var a=x.responseText;console.log(a);id=a.substr(0,a.indexOf('\\n'));z=a.substr(a.indexOf('\\n')+1);if(z.length>0){t.value+=z;}t.scrollTop=99999;sn=t.scrollTop;}};x.open('GET','cs?c2='+id+o,true);x.send();}lt=setTimeout(f,2345);return false;}window.addEventListener('load', f);";
const char ADVANCED_CONFIG_SCRIPT[] PROGMEM =
    "function setup_click(eid){var ec=document.getElementById(eid);ec.onclick=function(){editElementDict(ec);};}"\
    "function dictTable(dict){var tb='<table id=\"current-table\" style=\"width:100%;border-collapse:collapse;background:var(--surface2);border-radius:var(--radius);overflow:hidden;margin:0.5rem 0;\">'; for(const [key, value] of Object.entries(dict)){var valuestr=value;if(typeof value=='string'){valuestr='&quot;'+value+'&quot;'};tb+='<tr style=\"border-bottom:1px solid var(--border);\"><td style=\"padding:0.4rem;color:var(--accent2);font-size:0.8rem;\">'+key+'</td><td style=\"padding:0.4rem;\"><input id=\"d-'+key+'\" type=\"text\" placeholder=\"'+valuestr+'\" value=\"'+valuestr+'\" style=\"width:100%;padding:0.35rem;background:var(--surface);color:var(--text);border:1px solid var(--border);border-radius:4px;font-size:0.8rem;\"><div class=\"em\"></div></td></tr>';}tb+='</table>';return tb;}"\
    "function tableDictString(tbl){var ts='{';for(let i=0;i<tbl.rows.length;i++){var row=tbl.rows[i];var rowid=row.innerText.replace(/[\\t\\r\\n]+/gm,'');ts+'\"'+rowid+'\":'+document.getElementById('d-'+rowid).value+','};ts=ts.slice(0,-1)+'}';return ts;}"\
    "function tableDoneHandler(btn){var tbd=document.getElementById('current-table');var ds=tableDictString(tbd);current_ctrl.value=ds;document.getElementById('dt-'+current_id).remove();current_ctrl=null;current_id=null;}"\
    "function editElementDict(ed){if(current_ctrl===null){var ph=ed.getAttribute('placeholder');var dstring=ed.value!=''?ed.value:ph;if(dstring!==''){current_id=ed.id;var dict=JSON.parse(dstring);var tblhtml='<div id=\"dt-'+current_id+'\">'+dictTable(dict)+'<input type=\"button\" value=\"Done\" onclick=\"tableDoneHandler()\" style=\"margin-top:0.4rem;width:auto;padding:0.4rem 1.2rem;\"></div>';ed.insertAdjacentHTML('afterend',tblhtml);current_ctrl=ed;}}}"\
    "var current_id,current_ctrl=null;window.addEventListener('load',function(){setup_click('modem_startup');});";
const char IOTWEBCONF_WORLDMAP_SCRIPT[] PROGMEM  ="var wmx=null,wmt;function wmf(p){var sp,mc,gs,lp;clearTimeout(wmt);wmx=new XMLHttpRequest();wmx.onreadystatechange=function() {if(wmx.readyState==4&&wmx.status==200){var wma=wmx.responseText;var wmp = wma.split(',');sp=document.getElementById('wmsatpos');sp.setAttribute('cx', wmp[0]);sp.setAttribute('cy', wmp[1]);mc=document.getElementById('modemconfig');for(let r=0;r<6;r++){mc.rows[r].cells[1].innerHTML=wmp[r+2]};if(wmp[2]=='LoRa'){mc.rows[3].cells[0].innerHTML='Spreading Factor ';mc.rows[4].cells[0].innerHTML='Coding Rate ';}else{mc.rows[3].cells[0].innerHTML='Bitrate ';mc.rows[4].cells[0].innerHTML='Frequency dev ';};gs=document.getElementById('gsstatus');for(let r=0;r<6;r++){gs.rows[r].cells[1].innerHTML=wmp[r+8];};sd=document.getElementById('satdata');for(let r=0;r<6;r++){sd.rows[r].cells[1].innerHTML=wmp[r+14];};lp=document.getElementById('lastpacket');for(let r=0;r<4;r++){lp.rows[r].cells[1].innerHTML=wmp[r+20];};lp.rows[4].cells[0].innerHTML=wmp[24];}};wmx.open('GET','wm',true);wmx.send();wmt=setTimeout(wmf,5000);return false;}window.addEventListener('load', wmf);";
const char IOTWEBCONF_CONFIG_STYLE_INNER[] PROGMEM = " fieldset[id='Board config'] div:nth-of-type(3) ~ div { display:none}";

// ---- Board Template Wizard: <dialog> + JS injected after the config form ----
const char BOARD_WIZARD_HTML[] PROGMEM =
  "<dialog id='bt-dlg' style='max-width:640px;width:95%;font-family:Arial;padding:20px;border-radius:8px;border:2px solid #888;box-shadow:0 4px 24px rgba(0,0,0,0.4)'>"
  "<h3 style='margin:0 0 4px'>Board Template Wizard</h3>"
  "<p style='margin:0 0 10px;font-size:0.85em;color:#555'>Pin 255 = not used / N/A. Values are pre-filled from the current template when available.</p>"
  "<div style='overflow-y:auto;max-height:62vh'>"
  "<table style='width:100%;border-collapse:collapse;font-size:0.88em'>"
  /* OLED header + enable row (same pattern as Ethernet) */
  "<tbody>"
  "<tr><th colspan='4' style='background:#2980b9;color:#fff;padding:5px 6px;text-align:left'>OLED Display</th></tr>"
  "<tr>"
  "<td style='padding:3px 5px'>Enable OLED</td>"
  "<td><input id='woled_en' type='checkbox' checked onchange='btOledTog()'></td>"
  "<td></td><td></td>"
  "</tr>"
  "</tbody>"
  /* OLED pin rows - hidden when disabled */
  "<tbody id='wi-oled'>"
  "<tr><td style='padding:3px 5px'>I2C Addr (dec, 60=0x3C)</td><td><input id='wa' type='number' value='60' min='0' max='127' style='width:68px'></td>"
  "<td style='padding:3px 5px'>SDA</td><td><input id='wb' type='number' value='21' min='0' max='255' style='width:68px'></td></tr>"
  "<tr><td style='padding:3px 5px'>SCL</td><td><input id='wc' type='number' value='22' min='0' max='255' style='width:68px'></td>"
  "<td style='padding:3px 5px'>RST&nbsp;(255=N/A)</td><td><input id='wd' type='number' value='255' min='0' max='255' style='width:68px'></td></tr>"
  "</tbody>"
  /* Board Pins */
  "<tbody><tr><th colspan='4' style='background:#2980b9;color:#fff;padding:5px 6px;text-align:left'>Board Pins</th></tr>"
  "<tr><td style='padding:3px 5px'>PROG Button</td><td><input id='we' type='number' value='39' min='0' max='255' style='width:68px'></td>"
  "<td style='padding:3px 5px'>Board LED</td><td><input id='wf' type='number' value='2' min='0' max='255' style='width:68px'></td></tr>"
  /* LoRa Radio header + model select */
  "<tr><th colspan='4' style='background:#2980b9;color:#fff;padding:5px 6px;text-align:left'>LoRa Radio</th></tr>"
  "<tr><td style='padding:3px 5px'>Radio model</td><td colspan='3'>"
  "<select id='wg' onchange='btRadioTog()' style='width:100%;padding:2px'>"
  "<option value='0'>None (disabled)</option>"
  "<option value='1'>SX1278 &mdash; 433 MHz</option>"
  "<option value='2'>SX1276 &mdash; 868/915 MHz</option>"
  "<option value='5'>SX1268</option>"
  "<option value='6'>SX1262</option>"
  "<option value='8'>SX1280 &mdash; 2.4 GHz</option>"
  "</select></td></tr></tbody>"
  /* Radio pin rows - hidden when None */
  "<tbody id='wi-radio'>"
  "<tr><td style='padding:3px 5px'>NSS / CS</td><td><input id='wh' type='number' value='18' min='0' max='255' style='width:68px'></td>"
  "<td style='padding:3px 5px'>DIO0</td><td><input id='wi' type='number' value='26' min='0' max='255' style='width:68px'></td></tr>"
  "<tr><td style='padding:3px 5px'>DIO1</td><td><input id='wj' type='number' value='33' min='0' max='255' style='width:68px'></td>"
  "<td style='padding:3px 5px'>BUSY&nbsp;(255=N/A)</td><td><input id='wk' type='number' value='255' min='0' max='255' style='width:68px'></td></tr>"
  "<tr><td style='padding:3px 5px'>RST</td><td><input id='wl' type='number' value='14' min='0' max='255' style='width:68px'></td>"
  "<td style='padding:3px 5px'>MISO</td><td><input id='wm' type='number' value='19' min='0' max='255' style='width:68px'></td></tr>"
  "<tr><td style='padding:3px 5px'>MOSI</td><td><input id='wn' type='number' value='27' min='0' max='255' style='width:68px'></td>"
  "<td style='padding:3px 5px'>SCK</td><td><input id='wo' type='number' value='5' min='0' max='255' style='width:68px'></td></tr>"
  "<tr><td style='padding:3px 5px'>TCXO Voltage (0=none)</td><td><input id='wp' type='number' value='0' min='0' max='3.3' step='0.1' style='width:68px'></td>"
  "<td style='padding:3px 5px'>RX_EN&nbsp;(255=N/A)</td><td><input id='wq' type='number' value='255' min='0' max='255' style='width:68px'></td></tr>"
  "<tr><td style='padding:3px 5px'>TX_EN&nbsp;(255=N/A)</td><td><input id='wr' type='number' value='255' min='0' max='255' style='width:68px'></td>"
  "<td></td><td></td></tr>"
#if !defined(CONFIG_IDF_TARGET_ESP32C3)
  "<tr><td style='padding:3px 5px'>SPI Bus</td><td colspan='3'>"
  "<select id='wlspi' style='padding:2px'>"
  "<option value='2'>SPI2 (default)</option>"
  "<option value='3'>SPI3</option>"
  "</select></td></tr>"
#endif
  "</tbody>"
  /* Ethernet */
  "<tbody><tr><th colspan='4' style='background:#2980b9;color:#fff;padding:5px 6px;text-align:left'>Ethernet (optional)</th></tr>"
  "<tr>"
  "<td style='padding:3px 5px'>Enable Ethernet</td>"
  "<td><input id='ws' type='checkbox' onchange='btEthTog()'></td>"
  "<td id='wt-lbl' style='padding:3px 5px;display:none'>PHY type</td>"
  "<td id='wt-td' style='display:none'><select id='wt' onchange='btEthPhyTog()' style='width:100%;padding:2px'><option value='0'>W5500</option><option value='1'>DM9051</option><option value='2'>KSZ8851SNL</option>"
#if defined(CONFIG_IDF_TARGET_ESP32)
  "<option value='255'>Internal EMAC (LAN8720 / IP101...)</option>"
#endif
  "</select></td>"
  "</tr></tbody>"
  "<tbody id='wi-eth-spi' style='display:none'>"
#if !defined(CONFIG_IDF_TARGET_ESP32C3)
  "<tr><td style='padding:3px 5px'>SPI Bus</td><td colspan='3'>"
  "<select id='wespi' style='padding:2px'>"
  "<option value='2'>SPI2 (default)</option>"
  "<option value='3'>SPI3</option>"
  "</select></td></tr>"
#endif
  "<tr><td style='padding:3px 5px'>CS</td><td><input id='wu' type='number' value='255' min='0' max='255' style='width:68px'></td>"
  "<td style='padding:3px 5px'>INT</td><td><input id='wv' type='number' value='255' min='0' max='255' style='width:68px'></td></tr>"
  "<tr><td style='padding:3px 5px'>RST</td><td><input id='ww' type='number' value='255' min='0' max='255' style='width:68px'></td>"
  "<td style='padding:3px 5px'>MISO</td><td><input id='wx' type='number' value='255' min='0' max='255' style='width:68px'></td></tr>"
  "<tr><td style='padding:3px 5px'>MOSI</td><td><input id='wy' type='number' value='255' min='0' max='255' style='width:68px'></td>"
  "<td style='padding:3px 5px'>SCK</td><td><input id='wz' type='number' value='255' min='0' max='255' style='width:68px'></td></tr>"
  "</tbody>"
#if defined(CONFIG_IDF_TARGET_ESP32)
  "<tbody id='wi-eth-emac' style='display:none'>"
  "<tr><td style='padding:3px 5px'>MDC</td><td><input id='wemdc' type='number' value='23' min='0' max='39' style='width:68px'></td>"
  "<td style='padding:3px 5px'>MDIO</td><td><input id='wemdio' type='number' value='18' min='0' max='39' style='width:68px'></td></tr>"
  "<tr><td style='padding:3px 5px'>PHY chip</td><td colspan='3'><select id='wephytype' style='width:100%;padding:2px'>"
  "<option value='0'>LAN8720</option><option value='1'>IP101</option><option value='2'>RTL8201</option>"
  "<option value='3'>DP83848</option><option value='4'>KSZ8041</option><option value='5'>KSZ8081</option>"
  "</select></td></tr>"
  "<tr><td style='padding:3px 5px'>PHY addr</td><td><input id='wephyaddr' type='number' value='1' min='0' max='31' style='width:68px'></td>"
  "<td style='padding:3px 5px'>REF_CLK pin</td><td><input id='werefclk' type='number' value='0' min='0' max='39' style='width:68px'></td></tr>"
  "<tr><td style='padding:3px 5px'>PHY drives CLK</td><td><input id='weclkext' type='checkbox' checked></td>"
  "<td style='padding:3px 5px'>PHY RST&nbsp;(255=N/A)</td><td><input id='wephyrst' type='number' value='255' min='0' max='39' style='width:68px'></td></tr>"
"<td style='padding:3px 5px'>OSC EN&nbsp;(255=N/A)</td><td><input id='weoscen' type='number' value='16' min='0' max='39' style='width:68px'></td></tr>"
  "</tbody>"
#endif
  "</table></div>"
  "<div style='margin-top:14px;display:flex;gap:8px'>"
  "<button type='button' onclick='btWzApply()' style='background:#27ae60;color:#fff;padding:8px 18px;border:none;border-radius:4px;cursor:pointer;font-size:0.95em'>&#10003;&nbsp;Apply to Board Template</button>"
  "<button type='button' onclick='btWzCancel()' style='padding:8px 18px;border-radius:4px;cursor:pointer;font-size:0.95em'>Cancel</button>"
  "</div></dialog>"
  "<script>"
  "function btOledTog(){"
  "var en=document.getElementById('woled_en').checked;"
  "document.getElementById('wi-oled').style.display=en?'table-row-group':'none';}"
  "function btRadioTog(){"
  "var d=document.getElementById('wg').value==='0';"
  "document.getElementById('wi-radio').style.display=d?'none':'table-row-group';}"
  "function btEthTog(){"
  "var en=document.getElementById('ws').checked;"
  "var ds=en?'':'none';"
  "document.getElementById('wt-lbl').style.display=ds;"
  "document.getElementById('wt-td').style.display=ds;"
  "if(en)btEthPhyTog();else{"
  "var spi=document.getElementById('wi-eth-spi');"
  "var emac=document.getElementById('wi-eth-emac');"
  "if(spi)spi.style.display='none';"
  "if(emac)emac.style.display='none';}}"
  "function btEthPhyTog(){"
  "var isEmac=document.getElementById('wt').value==='255';"
  "var spi=document.getElementById('wi-eth-spi');"
  "var emac=document.getElementById('wi-eth-emac');"
  "if(spi)spi.style.display=isEmac?'none':'table-row-group';"
  "if(emac)emac.style.display=isEmac?'table-row-group':'none';}"
  "function btWzCancel(){document.getElementById('bt-dlg').close();}"
  "function btWzOpen(){"
  "var t=document.getElementById('board_template').value;"
  "var j=null;"
  "if(t){try{j=JSON.parse(t);}catch(e){}}"
  "if(!j){var bi=parseInt(document.getElementById('board').value);"
  "j=(typeof boardDefaults!=='undefined'&&boardDefaults[bi])||null;}"
  "if(j){"
  "function gv(k,d){return j.hasOwnProperty(k)?j[k]:d;}"
  "function sv(i,v){document.getElementById(i).value=v;}"
  "var oledEN=(gv('aADDR',60)!==0);"
  "document.getElementById('woled_en').checked=oledEN;"
  "if(oledEN){sv('wa',gv('aADDR',60));sv('wb',gv('oSDA',21));sv('wc',gv('oSCL',22));sv('wd',gv('oRST',255));}"
  "sv('we',gv('pBut',39));sv('wf',gv('led',2));"
  "var rv=gv('radio',1);sv('wg',rv);"
  "if(rv!==0){sv('wh',gv('lNSS',18));sv('wi',gv('lDIO0',26));sv('wj',gv('lDIO1',33));sv('wk',gv('lBUSSY',255));"
  "sv('wl',gv('lRST',14));sv('wm',gv('lMISO',19));sv('wn',gv('lMOSI',27));sv('wo',gv('lSCK',5));"
  "sv('wp',gv('lTCXOV',0));sv('wq',gv('RXEN',255));sv('wr',gv('TXEN',255));"
#if !defined(CONFIG_IDF_TARGET_ESP32C3)
  "sv('wlspi',gv('lSPI',2));"
#endif
  "}"
  "var en=gv('ethEN',false);document.getElementById('ws').checked=en;"
  "if(en){var phy=gv('ethPHY',0);sv('wt',phy);"
  "if(phy===255){"
#if defined(CONFIG_IDF_TARGET_ESP32)
  "sv('wemdc',gv('ethMDC',23));sv('wemdio',gv('ethMDIO',18));"
  "sv('wephyaddr',gv('ethPhyAddr',1));sv('wephytype',gv('ethPhyType',0));"
  "sv('werefclk',gv('ethRefClk',0));document.getElementById('weclkext').checked=gv('ethClkExt',true);"
  "sv('wephyrst',gv('ethPhyRST',255));"
  "sv('weoscen',gv('ethOscEN',255));"
#endif
  "}else{"
  "sv('wu',gv('ethCS',255));sv('wv',gv('ethINT',255));"
  "sv('ww',gv('ethRST',255));sv('wx',gv('ethMISO',255));sv('wy',gv('ethMOSI',255));sv('wz',gv('ethSCK',255));"
#if !defined(CONFIG_IDF_TARGET_ESP32C3)
  "sv('wespi',gv('ethSPI',2));"
#endif
  "}}"
  "}"
  "btOledTog();btRadioTog();btEthTog();"
  "document.getElementById('bt-dlg').showModal();}"
  "function btWzApply(){"
  "var ni=Number,fi=parseFloat,v=function(i){return document.getElementById(i).value;};"
  "var oledDis=!document.getElementById('woled_en').checked;"
  "var radioDis=document.getElementById('wg').value==='0';"
  "var o={};"
  "if(oledDis){o.aADDR=0;o.oSDA=255;o.oSCL=255;}"
  "else{o.aADDR=ni(v('wa'));o.oSDA=ni(v('wb'));o.oSCL=ni(v('wc'));"
  "var oRST=ni(v('wd'));if(oRST!==255)o.oRST=oRST;}"
  "o.pBut=ni(v('we'));o.led=ni(v('wf'));"
  "if(radioDis){o.radio=0;o.lNSS=255;o.lDIO0=255;o.lDIO1=255;"
  "o.lBUSSY=255;o.lRST=255;o.lMISO=255;o.lMOSI=255;o.lSCK=255;o.lTCXOV=0;}"
  "else{o.radio=ni(v('wg'));o.lNSS=ni(v('wh'));o.lDIO0=ni(v('wi'));o.lDIO1=ni(v('wj'));"
  "o.lBUSSY=ni(v('wk'));o.lRST=ni(v('wl'));o.lMISO=ni(v('wm'));o.lMOSI=ni(v('wn'));"
  "o.lSCK=ni(v('wo'));o.lTCXOV=fi(v('wp'));"
  "var rx=ni(v('wq'));if(rx!==255)o.RXEN=rx;"
  "var tx=ni(v('wr'));if(tx!==255)o.TXEN=tx;"
#if !defined(CONFIG_IDF_TARGET_ESP32C3)
  "o.lSPI=ni(v('wlspi'));"
#endif
  "}"
  "if(document.getElementById('ws').checked){"
  "var phy=ni(v('wt'));o.ethEN=true;o.ethPHY=phy;"
  "if(phy===255){"
#if defined(CONFIG_IDF_TARGET_ESP32)
  "o.ethMDC=ni(v('wemdc'));o.ethMDIO=ni(v('wemdio'));"
  "o.ethPhyAddr=ni(v('wephyaddr'));o.ethPhyType=ni(v('wephytype'));"
  "o.ethRefClk=ni(v('werefclk'));o.ethClkExt=document.getElementById('weclkext').checked;"
  "var emacRst=ni(v('wephyrst'));if(emacRst!==255)o.ethPhyRST=emacRst;"
  "var emacOscEN=ni(v('weoscen'));if(emacOscEN!==255)o.ethOscEN=emacOscEN;"
#endif
  "}else{"
  "o.ethCS=ni(v('wu'));o.ethINT=ni(v('wv'));o.ethRST=ni(v('ww'));"
  "o.ethMISO=ni(v('wx'));o.ethMOSI=ni(v('wy'));o.ethSCK=ni(v('wz'));"
#if !defined(CONFIG_IDF_TARGET_ESP32C3)
  "o.ethSPI=ni(v('wespi'));"
#endif
  "}}"
  "if(!radioDis&&document.getElementById('ws').checked&&ni(v('wt'))!==255){"
  "var lsE=document.getElementById('wlspi'),esE=document.getElementById('wespi');"
  "var sharedBus=(!lsE||!esE||lsE.value===esE.value);"
  "if(sharedBus){"
  "if(o.lMISO!==o.ethMISO||o.lMOSI!==o.ethMOSI||o.lSCK!==o.ethSCK){"
  "alert('Radio y Ethernet comparten el mismo bus SPI.\\nMISO, MOSI y SCK deben ser iguales en ambas secciones.');"
  "return;}"
  "if(o.lNSS===o.ethCS){"
  "alert('Radio y Ethernet comparten el mismo bus SPI.\\nNSS (radio) y CS (ethernet) deben ser pines diferentes.');"
  "return;}"
  "}}"
  "document.getElementById('board_template').value=JSON.stringify(o);"
  "document.getElementById('tpl_dirty').value='1';"
  "document.getElementById('bt-dlg').close();}"
  "</script>";

#endif // TINYGS_HTML_H