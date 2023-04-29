/*
    2023 AT-Krypt Technologies. 
    Version 4.3

    All rights reserved ©
    AT-Krypt Technologies
*/

const letters = [
    [" button:hover123", ')"!JFAPvam*)2#', "i2K9(cac)a2#^<", "oAlrmvaSk914?=", "gmaND8*md=12k;", "381>>fkoeLaE^^", "qQfos4%%#13aT5", "^hp9d,*avisu3m", ")cMa4AS?+A;:d5"],
    [" button:hover1234",')4mcakI', "laOsIf/", "KAc75#F", "am34*4)", "/39d_:d", "lafDs($", "L.aAOLa", "Ap*~23a"],
    [' ', "aD0Fp", "9wvfd", "aMRIR"],
    ['a', "kG.Bz", "^sdaG", "=A!kd"], ['A', "e:3,n", "SmYca", "KGCj3"],
    ['b', "OaO9v", "Skc?l", "kF.m?"], ['B', "1eIFb", "0tWos", "?#Dfa"],
    ['c', "jHy$j", "^)ISa", "LCk/4"], ['C', "9KE?a", "IJIol", "dBla*"],
    ['d', "BVa6d", "MvAQ/", ":F:3v"], ['D', "WTS]%", "!2}F]", "n/RrC"],
    ['e', "aHz#s", "*b^Aa", "77%qa"], ['E', "KPv7/", "<pb#s", "mfcA0"], ['€', "Oo(1)", "&%o4K", "_.AgB"],
    ['f', "lW$]!", ">0dak", "odBas"], ['F', "=TzSa", "/EnRa", "^3bVv"],
    ['g', "dS-p)", "pa<by", "=Fl,s"], ['G', "iJ!k4", "PA3bO", "gGmca"],
    ['h', "TR_O%", "SAm,0", "KGv43"], ['H', "=n=2A", "vInd$", "dfbo%"],
    ['i', "oad+S", "$.$5f", "0Abfg"], ['I', "fgs(d", "0sad?", "ffm00"],
    ['j', "KDJa%", "?=a%s", "SADhO"], ['J', "kadS#", "FAk3p", "KF/b/"],
    ['k', "lOMk=", "&&=_s", "*'*ad"], ['K', "JHGa9", "KHcc9", "Kv322"],
    ['l', "(3_d=", "+J+12", "49yAd"], ['L', "-Ts_-", "0GMXZ", ":sBa;"],
    ['m', "a.Rbr", "FKAJc", "QbYT4"], ['M', "h28^2", "FM4Al", "_-F-a"],
    ['n', "G_2T4", "Mr=p$", "d3(a)"], ['N', "r=UPI", "kJs?+", "pa<s2"], 
    ['o', "pisQ2", "`U`sa", "PLFa3"], ['O', "dOFf4", "<^2-!", ".3:AO"],
    ['p', "Mk!J3", "<pq>4", "fo8s="], ['P', "KRLN4", "*VzTv", "FkF32"],
    ['q', "#2d,d", "pL+am", "vm0IA"], ['Q', "=0}cC", "tFJvm", "ofM>>"],
    ['r', "MN.AB", "a?0!+", "0fZFA"], ['R', "DR>2k", "}]Za0", ")AXad"],
    ['s', "fN:al", "KHjxj", "FHvga"], ['S', "K8PP2", "[4{oP", "DIVKa"],
    ['t', "71m2D", "@_@KJ", "_Dya:"], ['T', ")dJCa", "Pb^3~", ";dQ1a"],
    ['u', "[ad$]", "`saSD", "~2aas"], ['U', "bgFNw", "~2#lA", "pfH?+"],
    ['v', "aALEd", "0Oa)s", "oYe]~"], ['V', "AOdRk", "''aGs", "K5K01"],
    ['w', "tr;ub", "kl(mn", ")d9ab"], ['W', "NEDrG", "x!!sa", "IDIgn"],
    ['x', "kurVi", "%5$#$", "tiha#"], ['X', "yet&&", "#_.&0", "mVI7A"],
    ['y', "ki4)R", "^@>rt", "@*aBs"], ['Y', "Ka#m8", "$TVWI", ")AS8a"],
    ['z', "94aKM", "MfVMU", "aF2dp"], ['Z', "tsa!2", "1P2{4", "Gv1O6"],
    ['0', "LOAV2", "09a87", "acv*)"], ['=', "KDji3", "KG%%f", "CX'A*"], ['}', "2F3vl", "*H_:a", "1viA@"],
    ['1', "9fa3s", "1:2f#", "MCP03"], ['!', "95dHs", "kL!3*", "A'Dv!"],
    ['2', "HKaf3", "99;fs", "a1BVx"], ['"', "iAvks", "mbA>4", "GFH*A"], ['@', "tar/m", "!qQ0p", "2@gia"],
    ['3', ")8j4s", "10a0b", "AmiGr"], ['#', "*3fa]", "0301p", "VM_af"], ['£', "pAu#", ">ZZ<", ")(d)Y"],
    ['4', "laOy3", "Lo%1!", "VMgcd"], ['$', "dASg9", "A!=a!", "ZXx50"], ['¤', "Ghz;2", "|2R@g", "10H2f"],
    ['5', "z<4ta", "tOPG[", "a0sg#"], ['%', "ko|53", "K<&7h", "|aS,%"],
    ['6', "39h_.", "SKOb)", "))ggh"], ['&', ",;g,4", "ka6))", "ld.%@"],
    ['7', "ksma}", "AKrMm", "bmfw("], ['/', "+/na1", ";_;kI", "02M.C"], ['{', "^#^a4", "1;::a", ",Ax23"],
    ['8', "*'aGa", "QqF%N", ":sarA"], ['(', "Lc1x4", "P90t=", ",as22"], ['[', "d1b/s", "/%/3s", "moa2V"],
    ['9', "0$nDk", "^AT~a", "A:>U<"], [')', "=l6fs", "+/_Mk", "<gv#r"], [']', "MG3as", "V2v2<", "ocas4"],
    ['+', "0fsaG", "II!8!", "da:m_"], ['?', "df39[", "P:A0=", ":s031"], ['\\', "k^=32", "A=A|B", "0v4mw"],
    ['-', "#odA*", "$18G(", "0Dq#m"], ['_', "d:as0", "B$=aa", "!_cas"],
    ['.', "VL9a*", "Q6WeR", "konAa"], [':', "Vk99)", "/{aH}", "MI3'*"],
    [',', "rf/(A", "OFv?R", "dmFas"], [';', "FLs:t", "rmNR?", "KKona"],
    ['*', "PmS66", "Ak+i2", "04mgf"], ["'", "++T#7", "MAosY", "mid#s"],
    ['¨', "77v7q", "*ak2^", "vsia/"], ['~', "Qo_q$", "2;:H1", "m_A,a"], ['^', "133C7", "72E:1", "gK?34"],
    ['´', "Fo7RT", "of,Da", "(DA6)"], ['`', "8:DqA", "Mn3ED", "/dts/"],
    ['½', "^3f0x", "2//1m", "9dd%k"], ['§', "Gomf^", "?+!%/", "ol*.?"],
    ['<', "Q3Qvl", "maji.", "0=B+1"], ['>', "F;Agv", "inMdo", "9adcm"], ['|', "D%&OF", "groFw", "9d5A5"],
    ['ö', "KL4S?", "alOHl", "9cm#{"], ['Ö', "#cxhl", "^*H|T", "VMg12"],
    ['å', "zi67p", "adF=i", "Ker]N"], ['Å', "Nit_E", "Ln86!", "ma&Me"],
    ['ä', "Q(d1a", "nu2oL", "Jo!45"], ['Ä', "Ha6KA", "ristO", "0)!hc"]
]
function atKrypt(cryptString, subCount){
    var encryptedFinalText;
    var maar = subCount;
    encryptedFinalText = "";
    var rndm2 = Math.floor(Math.random() * 8) + 1; 
    var rndm3 = Math.floor(Math.random() * 8) + 1; 
    var enkryptattava = cryptString;
    encryptedFinalText = letters[0][rndm2] + encryptedFinalText
    for(let o=0; o<maar ;o++) {
        for(let i = 0; i < enkryptattava.length; i++){
            let currentLetter = enkryptattava.charAt(i);
            let rndm = Math.floor(Math.random() * 3) + 1;
            for(let j = 0; j < letters.length; j++){ 
                if(currentLetter == letters[j][0]){
                    encryptedFinalText += letters[j][rndm];
                }
            }
        }
        encryptedFinalText += letters [1][rndm3];
        enkryptattava = encryptedFinalText;
    }
    encryptedFinalText += maar;
    return encryptedFinalText;
}
function atDekrypt(decryptString){
    var decryptFinalText;
    var hkmpv = 2^53;
    decryptFinalText = "";
    var dekryptattava = decryptString;
    var currentChars = "";
    var maara = dekryptattava.slice(dekryptattava.length - 1, dekryptattava.length);
    var decLe = dekryptattava.length-7;
    dekryptattava = dekryptattava.substring(0, decLe)
    dekryptattava = dekryptattava.substring(14);
    for (let u = 0; u < maara; u++){
        for(let i = 0; i < dekryptattava.length*hkmpv; i++){
            currentChars = dekryptattava.substring(0, 5);
            dekryptattava = dekryptattava.substring(5);
            for(let j = 0; j < letters.length; j++) {
                if(currentChars == letters[j][1] || currentChars == letters[j][2] || currentChars == letters[j][3]){
                    decryptFinalText += letters[j][0];
                }
            }
        }
        dekryptattava = decryptFinalText;
    }
    return decryptFinalText;
}
module.exports = {
    atKrypt: atKrypt,
    atDekrypt: atDekrypt
};