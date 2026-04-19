/**
 * atOS chess program - simple chess application demonstrating ATGL
 * Part of atOS
 * Licensed under the MIT License. See LICENSE file in the project root for full license information.
 */

#include <LIBRARIES/ATGL/ATGL.h>
#include <STD/PROC_COM.h>
#include <STD/FS_DISK.h>
#include <STD/STRING.h>
#include <STD/MEM.h>
#include <STD/DEBUG.h>

#define ROWS 8
#define COLS 8

#define WHITE_PAWN   'P'
#define BLACK_PAWN   'p'
#define WHITE_ROOK   'R'
#define BLACK_ROOK   'r'
#define WHITE_KNIGHT 'N'
#define BLACK_KNIGHT 'n'
#define WHITE_BISHOP 'B'
#define BLACK_BISHOP 'b'
#define WHITE_QUEEN  'Q'
#define BLACK_QUEEN  'q'
#define WHITE_KING   'K'
#define BLACK_KING   'k'
#define EMPTY        ' '
#define MOVE_MARKER  '*'

#define WHITE_TURN 0
#define BLACK_TURN 1

#define MOVE_LEGAL   TRUE
#define MOVE_ILLEGAL FALSE

#define MAX_MOVE_HISTORY 128
#define HISTORY_DISPLAY_ROWS 14

static int chess_turn = WHITE_TURN;

/* ---- Forward declarations ---- */
VOID RESET_CALLBACK(PATGL_NODE node);
VOID SWITCH_MODE_CALLBACK(PATGL_NODE node);
VOID MOVEMENT_CALLBACK(PATGL_NODE node);
void ai_make_move();

/* ============================================================
   Game state
   ============================================================ */

U8 chessboard[ROWS][COLS] = {
    { BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK },
    { BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN },
    { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
    { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
    { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
    { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
    { WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN },
    { WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK }
};

const U8 initial_chessboard[ROWS][COLS] = {
    { BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK },
    { BLACK_PAWN, BLACK_PAWN,   BLACK_PAWN,   BLACK_PAWN,  BLACK_PAWN, BLACK_PAWN,   BLACK_PAWN,   BLACK_PAWN  },
    { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
    { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
    { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
    { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
    { WHITE_PAWN, WHITE_PAWN,   WHITE_PAWN,   WHITE_PAWN,  WHITE_PAWN, WHITE_PAWN,   WHITE_PAWN,   WHITE_PAWN  },
    { WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK  }
};

/* Castling rights [0]=white KS, [1]=white QS, [2]=black KS, [3]=black QS */
static BOOL8 castling_rights[4] = { TRUE, TRUE, TRUE, TRUE };

/* En passant: col of pawn that just double-pushed, row of that pawn (-1 = none) */
static int en_passant_col = -1;
static int en_passant_row = -1;

/* Currently selected square (-1 if none) */
static int selected_row = -1;
static int selected_col = -1;

/* Move history */
static U8 move_history[MAX_MOVE_HISTORY][16];
static int move_history_count = 0;

/* Game over flag */
static BOOL8 game_over = FALSE;

/* Movement table */
U8 piece_movement_table[ROWS][COLS];

/* ============================================================
   UI nodes
   ============================================================ */
ATGL_NODE *board_nodes[ROWS][COLS];
ATGL_NODE *board_root        = NULL;
ATGL_NODE *num_rows_node     = NULL;
ATGL_NODE *txt_cols_node     = NULL;
ATGL_NODE *MAIN_PANEL        = NULL;
ATGL_NODE *RIGHT_PANEL       = NULL;
ATGL_NODE *RESET_BTN         = NULL;
ATGL_NODE *SWITCH_MODE_BTN   = NULL;
ATGL_NODE *mode_label_node   = NULL;
ATGL_NODE *turn_label_node   = NULL;
ATGL_NODE *status_label_node = NULL;
ATGL_NODE *history_labels[HISTORY_DISPLAY_ROWS];

typedef enum { MODE_PVP, MODE_PVAI } CHESS_MODE;
CHESS_MODE current_mode = MODE_PVP;

/* ============================================================
   Helper macros
   ============================================================ */
#define IS_WHITE(p) ((p) >= 'A' && (p) <= 'Z')
#define IS_BLACK(p) ((p) >= 'a' && (p) <= 'z')
#define IS_PIECE(p) ((p) != EMPTY && (p) != MOVE_MARKER)
#define SAME_COLOR(a,b) ((IS_WHITE(a) && IS_WHITE(b)) || (IS_BLACK(a) && IS_BLACK(b)))
#define ENEMY(a,b)   (IS_PIECE(a) && IS_PIECE(b) && !SAME_COLOR(a,b))
#define IN_BOARD(r,c) ((r)>=0 && (r)<ROWS && (c)>=0 && (c)<COLS)

/* ============================================================
   Movement table helpers
   ============================================================ */
VOID CLEAR_PIECE_MOVEMENT_TABLE() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            piece_movement_table[r][c] = MOVE_ILLEGAL;
}

static VOID mark_rook_rays(int fr, int fc, U8 p) {
    int dr[] = {-1, 1,  0, 0};
    int dc[] = { 0, 0, -1, 1};
    for (int d = 0; d < 4; d++) {
        int r = fr+dr[d], c = fc+dc[d];
        while (IN_BOARD(r,c)) {
            if (chessboard[r][c] == EMPTY) { piece_movement_table[r][c] = MOVE_LEGAL; }
            else { if (ENEMY(p, chessboard[r][c])) piece_movement_table[r][c] = MOVE_LEGAL; break; }
            r+=dr[d]; c+=dc[d];
        }
    }
}

static VOID mark_bishop_rays(int fr, int fc, U8 p) {
    int dr[] = {-1,-1, 1, 1};
    int dc[] = {-1, 1,-1, 1};
    for (int d = 0; d < 4; d++) {
        int r = fr+dr[d], c = fc+dc[d];
        while (IN_BOARD(r,c)) {
            if (chessboard[r][c] == EMPTY) { piece_movement_table[r][c] = MOVE_LEGAL; }
            else { if (ENEMY(p, chessboard[r][c])) piece_movement_table[r][c] = MOVE_LEGAL; break; }
            r+=dr[d]; c+=dc[d];
        }
    }
}

VOID CALCULATE_PIECE_MOVEMENT_TABLE(int fr, int fc) {
    CLEAR_PIECE_MOVEMENT_TABLE();
    U8 piece = chessboard[fr][fc];
    if (!IS_PIECE(piece)) return;

    switch (piece) {
        case WHITE_PAWN: {
            if (fr>0 && chessboard[fr-1][fc]==EMPTY) {
                piece_movement_table[fr-1][fc]=MOVE_LEGAL;
                if (fr==6 && chessboard[fr-2][fc]==EMPTY) piece_movement_table[fr-2][fc]=MOVE_LEGAL;
            }
            if (fr>0 && fc>0      && IS_BLACK(chessboard[fr-1][fc-1])) piece_movement_table[fr-1][fc-1]=MOVE_LEGAL;
            if (fr>0 && fc<COLS-1 && IS_BLACK(chessboard[fr-1][fc+1])) piece_movement_table[fr-1][fc+1]=MOVE_LEGAL;
            if (en_passant_row==fr && en_passant_col>=0) {
                if (fc>0      && en_passant_col==fc-1) piece_movement_table[fr-1][fc-1]=MOVE_LEGAL;
                if (fc<COLS-1 && en_passant_col==fc+1) piece_movement_table[fr-1][fc+1]=MOVE_LEGAL;
            }
        } break;
        case BLACK_PAWN: {
            if (fr<ROWS-1 && chessboard[fr+1][fc]==EMPTY) {
                piece_movement_table[fr+1][fc]=MOVE_LEGAL;
                if (fr==1 && chessboard[fr+2][fc]==EMPTY) piece_movement_table[fr+2][fc]=MOVE_LEGAL;
            }
            if (fr<ROWS-1 && fc>0      && IS_WHITE(chessboard[fr+1][fc-1])) piece_movement_table[fr+1][fc-1]=MOVE_LEGAL;
            if (fr<ROWS-1 && fc<COLS-1 && IS_WHITE(chessboard[fr+1][fc+1])) piece_movement_table[fr+1][fc+1]=MOVE_LEGAL;
            if (en_passant_row==fr && en_passant_col>=0) {
                if (fc>0      && en_passant_col==fc-1) piece_movement_table[fr+1][fc-1]=MOVE_LEGAL;
                if (fc<COLS-1 && en_passant_col==fc+1) piece_movement_table[fr+1][fc+1]=MOVE_LEGAL;
            }
        } break;
        case WHITE_ROOK: case BLACK_ROOK: mark_rook_rays(fr,fc,piece); break;
        case WHITE_KNIGHT: case BLACK_KNIGHT: {
            int m[8][2]={{-2,-1},{-2,1},{2,-1},{2,1},{-1,-2},{-1,2},{1,-2},{1,2}};
            for(int i=0;i<8;i++){int r=fr+m[i][0],c=fc+m[i][1];if(IN_BOARD(r,c)&&!SAME_COLOR(piece,chessboard[r][c]))piece_movement_table[r][c]=MOVE_LEGAL;}
        } break;
        case WHITE_BISHOP: case BLACK_BISHOP: mark_bishop_rays(fr,fc,piece); break;
        case WHITE_QUEEN: case BLACK_QUEEN:
            mark_rook_rays(fr,fc,piece); mark_bishop_rays(fr,fc,piece); break;
        case WHITE_KING: case BLACK_KING: {
            for(int dr=-1;dr<=1;dr++) for(int dc=-1;dc<=1;dc++){
                if(dr==0&&dc==0) continue;
                int r=fr+dr,c=fc+dc;
                if(IN_BOARD(r,c)&&!SAME_COLOR(piece,chessboard[r][c])) piece_movement_table[r][c]=MOVE_LEGAL;
            }
            /* Castling */
            if(piece==WHITE_KING&&fr==7&&fc==4){
                if(castling_rights[0]&&chessboard[7][5]==EMPTY&&chessboard[7][6]==EMPTY) piece_movement_table[7][6]=MOVE_LEGAL;
                if(castling_rights[1]&&chessboard[7][3]==EMPTY&&chessboard[7][2]==EMPTY&&chessboard[7][1]==EMPTY) piece_movement_table[7][2]=MOVE_LEGAL;
            }
            if(piece==BLACK_KING&&fr==0&&fc==4){
                if(castling_rights[2]&&chessboard[0][5]==EMPTY&&chessboard[0][6]==EMPTY) piece_movement_table[0][6]=MOVE_LEGAL;
                if(castling_rights[3]&&chessboard[0][3]==EMPTY&&chessboard[0][2]==EMPTY&&chessboard[0][1]==EMPTY) piece_movement_table[0][2]=MOVE_LEGAL;
            }
        } break;
        default: break;
    }
}

/* ============================================================
   Check detection
   ============================================================ */
static BOOL8 is_in_check(U8 board[ROWS][COLS], int side) {
    U8 king=(side==WHITE_TURN)?WHITE_KING:BLACK_KING;
    int kr=-1,kc=-1;
    for(int r=0;r<ROWS&&kr==-1;r++) for(int c=0;c<COLS&&kr==-1;c++) if(board[r][c]==king){kr=r;kc=c;}
    if(kr==-1) return TRUE;
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++){
        U8 p=board[r][c]; if(!IS_PIECE(p)) continue;
        if(side==WHITE_TURN&&IS_WHITE(p)) continue;
        if(side==BLACK_TURN&&IS_BLACK(p)) continue;
        if(p==BLACK_PAWN&&r+1==kr&&(c-1==kc||c+1==kc)) return TRUE;
        if(p==WHITE_PAWN&&r-1==kr&&(c-1==kc||c+1==kc)) return TRUE;
        if(p==WHITE_KNIGHT||p==BLACK_KNIGHT){int km[8][2]={{-2,-1},{-2,1},{2,-1},{2,1},{-1,-2},{-1,2},{1,-2},{1,2}};for(int i=0;i<8;i++)if(r+km[i][0]==kr&&c+km[i][1]==kc)return TRUE;}
        if(p==WHITE_ROOK||p==BLACK_ROOK||p==WHITE_QUEEN||p==BLACK_QUEEN){
            int dr[]={-1,1,0,0},dc[]={0,0,-1,1};
            for(int d=0;d<4;d++){int rr=r+dr[d],cc=c+dc[d];while(IN_BOARD(rr,cc)){if(rr==kr&&cc==kc)return TRUE;if(board[rr][cc]!=EMPTY)break;rr+=dr[d];cc+=dc[d];}}
        }
        if(p==WHITE_BISHOP||p==BLACK_BISHOP||p==WHITE_QUEEN||p==BLACK_QUEEN){
            int dr[]={-1,-1,1,1},dc[]={-1,1,-1,1};
            for(int d=0;d<4;d++){int rr=r+dr[d],cc=c+dc[d];while(IN_BOARD(rr,cc)){if(rr==kr&&cc==kc)return TRUE;if(board[rr][cc]!=EMPTY)break;rr+=dr[d];cc+=dc[d];}}
        }
        if((p==WHITE_KING||p==BLACK_KING)&&r-1<=kr&&kr<=r+1&&c-1<=kc&&kc<=c+1&&!(r==kr&&c==kc)) return TRUE;
    }
    return FALSE;
}

static BOOL8 move_leaves_in_check(int fr, int fc, int tr, int tc) {
    U8 tmp[ROWS][COLS];
    MEMCPY(tmp, chessboard, sizeof(chessboard));
    U8 piece=tmp[fr][fc];
    tmp[tr][tc]=piece; tmp[fr][fc]=EMPTY;
    if((piece==WHITE_PAWN||piece==BLACK_PAWN)&&fc!=tc&&chessboard[tr][tc]==EMPTY) tmp[fr][tc]=EMPTY;
    return is_in_check(tmp, IS_WHITE(piece)?WHITE_TURN:BLACK_TURN);
}

VOID FILTER_LEGAL_MOVES(int fr, int fc) {
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++)
        if(piece_movement_table[r][c]==MOVE_LEGAL && move_leaves_in_check(fr,fc,r,c))
            piece_movement_table[r][c]=MOVE_ILLEGAL;
}

static BOOL8 has_any_legal_move(int side) {
    int sr=selected_row, sc=selected_col; BOOL8 found=FALSE;
    for(int r=0;r<ROWS&&!found;r++) for(int c=0;c<COLS&&!found;c++){
        U8 p=chessboard[r][c]; if(!IS_PIECE(p)) continue;
        if(side==WHITE_TURN&&!IS_WHITE(p)) continue;
        if(side==BLACK_TURN&&!IS_BLACK(p)) continue;
        selected_row=r; selected_col=c;
        CALCULATE_PIECE_MOVEMENT_TABLE(r,c); FILTER_LEGAL_MOVES(r,c);
        for(int tr=0;tr<ROWS&&!found;tr++) for(int tc=0;tc<COLS&&!found;tc++)
            if(piece_movement_table[tr][tc]==MOVE_LEGAL) found=TRUE;
    }
    selected_row=sr; selected_col=sc; return found;
}

/* ============================================================
   Board color + UI sync
   ============================================================ */
VOID CLEAR_MOVE_MARKERS() {
    CLEAR_PIECE_MOVEMENT_TABLE();
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++){
        if(board_nodes[r][c]->text[1]=='*')
            board_nodes[r][c]->text[1]='\0';
        else if(board_nodes[r][c]->text[0]==MOVE_MARKER)
            board_nodes[r][c]->text[0]=EMPTY;
    }
}

VOID DRAW_MOVE_MARKERS(int fr, int fc) {
    CLEAR_MOVE_MARKERS();
    CALCULATE_PIECE_MOVEMENT_TABLE(fr,fc);
    FILTER_LEGAL_MOVES(fr,fc);
    /* Mark selected square */
    board_nodes[fr][fc]->text[0]=chessboard[fr][fc];
    board_nodes[fr][fc]->text[1]='*';
    board_nodes[fr][fc]->text[2]='\0';
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++)
        if(piece_movement_table[r][c]==MOVE_LEGAL){
            if(chessboard[r][c]==EMPTY){
                board_nodes[r][c]->text[0]='*';
                board_nodes[r][c]->text[1]='\0';
            } else {
                board_nodes[r][c]->text[0]=chessboard[r][c];
                board_nodes[r][c]->text[1]='*';
                board_nodes[r][c]->text[2]='\0';
            }
        }
}

/* ============================================================
   Move history
   ============================================================ */
static VOID push_move_history(U8 piece, int fr, int fc, int tr, int tc,
                               BOOL8 capture, BOOL8 ks, BOOL8 qs) {
    if(move_history_count>=MAX_MOVE_HISTORY){
        for(int i=0;i<MAX_MOVE_HISTORY-1;i++) MEMCPY(move_history[i],move_history[i+1],16);
        move_history_count=MAX_MOVE_HISTORY-1;
    }
    U8 *buf=move_history[move_history_count];
    if(ks) SNPRINTF((char*)buf,16,"O-O");
    else if(qs) SNPRINTF((char*)buf,16,"O-O-O");
    else SNPRINTF((char*)buf,16,"%c%c%c%c%c%c",piece,'a'+fc,'1'+(ROWS-1-fr),capture?'x':'-','a'+tc,'1'+(ROWS-1-tr));
    move_history_count++;
}

static VOID update_history_labels() {
    int start=move_history_count-HISTORY_DISPLAY_ROWS; if(start<0)start=0;
    for(int i=0;i<HISTORY_DISPLAY_ROWS;i++){
        int idx=start+i;
        ATGL_NODE_SET_TEXT(history_labels[i], idx<move_history_count?(char*)move_history[idx]:"");
    }
}

/* ============================================================
   Apply a move
   ============================================================ */
static VOID apply_move(int fr, int fc, int tr, int tc) {
    U8 piece=chessboard[fr][fc];
    BOOL8 capture=IS_PIECE(chessboard[tr][tc]);
    BOOL8 ks=FALSE, qs=FALSE;

    /* En passant capture */
    if((piece==WHITE_PAWN||piece==BLACK_PAWN)&&fc!=tc&&chessboard[tr][tc]==EMPTY){
        capture=TRUE;
        chessboard[fr][tc]=EMPTY;
        board_nodes[fr][tc]->text[0]=EMPTY; board_nodes[fr][tc]->text[1]='\0';
    }

    /* Castling */
    if(piece==WHITE_KING&&fr==7&&fc==4){
        if(tc==6){ks=TRUE;chessboard[7][5]=WHITE_ROOK;chessboard[7][7]=EMPTY;
            board_nodes[7][5]->text[0]=WHITE_ROOK;board_nodes[7][5]->text[1]='\0';
            board_nodes[7][7]->text[0]=EMPTY;     board_nodes[7][7]->text[1]='\0';}
        else if(tc==2){qs=TRUE;chessboard[7][3]=WHITE_ROOK;chessboard[7][0]=EMPTY;
            board_nodes[7][3]->text[0]=WHITE_ROOK;board_nodes[7][3]->text[1]='\0';
            board_nodes[7][0]->text[0]=EMPTY;     board_nodes[7][0]->text[1]='\0';}
        castling_rights[0]=FALSE; castling_rights[1]=FALSE;
    }
    if(piece==BLACK_KING&&fr==0&&fc==4){
        if(tc==6){ks=TRUE;chessboard[0][5]=BLACK_ROOK;chessboard[0][7]=EMPTY;
            board_nodes[0][5]->text[0]=BLACK_ROOK;board_nodes[0][5]->text[1]='\0';
            board_nodes[0][7]->text[0]=EMPTY;     board_nodes[0][7]->text[1]='\0';}
        else if(tc==2){qs=TRUE;chessboard[0][3]=BLACK_ROOK;chessboard[0][0]=EMPTY;
            board_nodes[0][3]->text[0]=BLACK_ROOK;board_nodes[0][3]->text[1]='\0';
            board_nodes[0][0]->text[0]=EMPTY;     board_nodes[0][0]->text[1]='\0';}
        castling_rights[2]=FALSE; castling_rights[3]=FALSE;
    }
    if(piece==WHITE_ROOK){if(fr==7&&fc==7)castling_rights[0]=FALSE;if(fr==7&&fc==0)castling_rights[1]=FALSE;}
    if(piece==BLACK_ROOK){if(fr==0&&fc==7)castling_rights[2]=FALSE;if(fr==0&&fc==0)castling_rights[3]=FALSE;}

    /* En passant state */
    en_passant_col=-1; en_passant_row=-1;
    if(piece==WHITE_PAWN&&fr==6&&tr==4){en_passant_col=fc;en_passant_row=4;}
    if(piece==BLACK_PAWN&&fr==1&&tr==3){en_passant_col=fc;en_passant_row=3;}

    /* Move */
    chessboard[tr][tc]=piece; chessboard[fr][fc]=EMPTY;
    if(piece==WHITE_PAWN&&tr==0) chessboard[tr][tc]=WHITE_QUEEN;
    if(piece==BLACK_PAWN&&tr==7) chessboard[tr][tc]=BLACK_QUEEN;

    board_nodes[fr][fc]->text[0]=EMPTY;                board_nodes[fr][fc]->text[1]='\0';
    board_nodes[tr][tc]->text[0]=chessboard[tr][tc];   board_nodes[tr][tc]->text[1]='\0';

    push_move_history(piece,fr,fc,tr,tc,capture,ks,qs);
    update_history_labels();
}

/* ============================================================
   Post-move game state
   ============================================================ */
static VOID check_game_state() {
    int side=chess_turn;
    BOOL8 chk=is_in_check(chessboard,side);
    BOOL8 moves=has_any_legal_move(side);
    if(chk&&!moves){ATGL_NODE_SET_TEXT(status_label_node,side==WHITE_TURN?"Checkmate! Black wins!":"Checkmate! White wins!");game_over=TRUE;}
    else if(!chk&&!moves){ATGL_NODE_SET_TEXT(status_label_node,"Stalemate! Draw.");game_over=TRUE;}
    else if(chk) ATGL_NODE_SET_TEXT(status_label_node,"Check!");
    else ATGL_NODE_SET_TEXT(status_label_node,"");
}

/* ============================================================
   Callbacks
   ============================================================ */
VOID RESET_CALLBACK(PATGL_NODE node) {
    MEMCPY(chessboard,initial_chessboard,sizeof(chessboard));
    castling_rights[0]=TRUE;castling_rights[1]=TRUE;castling_rights[2]=TRUE;castling_rights[3]=TRUE;
    en_passant_col=-1;en_passant_row=-1;selected_row=-1;selected_col=-1;
    chess_turn=WHITE_TURN;game_over=FALSE;move_history_count=0;
    CLEAR_MOVE_MARKERS();
    ATGL_NODE_SET_TEXT(turn_label_node,   "Turn: White");
    ATGL_NODE_SET_TEXT(mode_label_node,   current_mode==MODE_PVP?"Mode: PvP":"Mode: PvAI");
    ATGL_NODE_SET_TEXT(status_label_node, "");
    update_history_labels();
}

VOID SWITCH_MODE_CALLBACK(PATGL_NODE node) {
    current_mode=(current_mode==MODE_PVP)?MODE_PVAI:MODE_PVP;
    RESET_CALLBACK(NULL);
}

/* ============================================================
   Movement callback
   ============================================================ */
VOID MOVEMENT_CALLBACK(PATGL_NODE node) {
    if(game_over) return;
    if(current_mode==MODE_PVAI&&chess_turn==BLACK_TURN) return;

    /* Find clicked square */
    int cr=-1,cc=-1;
    for(int r=0;r<ROWS&&cr==-1;r++) for(int c=0;c<COLS&&cr==-1;c++) if(board_nodes[r][c]==node){cr=r;cc=c;}
    if(cr==-1) return;

    U8 cp=chessboard[cr][cc];

    if(selected_row>=0){
        /* Deselect */
        if(cr==selected_row&&cc==selected_col){selected_row=-1;selected_col=-1;CLEAR_MOVE_MARKERS();return;}
        /* Try move */
        if(piece_movement_table[cr][cc]==MOVE_LEGAL){
            apply_move(selected_row,selected_col,cr,cc);
            selected_row=-1;selected_col=-1;CLEAR_MOVE_MARKERS();
            chess_turn=1-chess_turn;
            ATGL_NODE_SET_TEXT(turn_label_node,chess_turn==WHITE_TURN?"Turn: White":"Turn: Black");
            check_game_state();
            if(!game_over&&current_mode==MODE_PVAI&&chess_turn==BLACK_TURN) ai_make_move();
            return;
        }
        /* Switch to another own piece */
        BOOL8 own=(chess_turn==WHITE_TURN&&IS_WHITE(cp))||(chess_turn==BLACK_TURN&&IS_BLACK(cp));
        if(own&&IS_PIECE(cp)){selected_row=cr;selected_col=cc;DRAW_MOVE_MARKERS(cr,cc);return;}
        /* Deselect */
        selected_row=-1;selected_col=-1;CLEAR_MOVE_MARKERS();return;
    }

    if(!IS_PIECE(cp)) return;
    BOOL8 own=(chess_turn==WHITE_TURN&&IS_WHITE(cp))||(chess_turn==BLACK_TURN&&IS_BLACK(cp));
    if(!own){DEBUG_PRINTF("Not your turn!\n");return;}
    selected_row=cr;selected_col=cc;
    DRAW_MOVE_MARKERS(cr,cc);
}

/* ============================================================
   AI stub (not touched as requested)
   ============================================================ */
void ai_make_move() {
    /* TODO: Implement AI move logic */
    DEBUG_PRINTF("AI makes a move (stub)\n");
    chess_turn=WHITE_TURN;
    ATGL_NODE_SET_TEXT(turn_label_node,"Turn: White");
    check_game_state();
}

/* ============================================================
   Entry point
   ============================================================ */
U32 ATGL_MAIN(U32 argc, PPU8 argv) {
    DISABLE_SHELL_KEYBOARD();
    ON_EXIT(ENABLE_SHELL_KEYBOARD);

    ATGL_INIT();
    ATGL_CREATE_SCREEN(ATGL_SA_NONE);

    PATGL_NODE root = ATGL_GET_SCREEN_ROOT_NODE();

    MAIN_PANEL = ATGL_CREATE_PANEL(root, (ATGL_RECT){10, 10, 790, 580}, ATGL_LAYOUT_NONE, 0, 0);

    num_rows_node = ATGL_CREATE_PANEL(MAIN_PANEL, (ATGL_RECT){ 0,  32,  32, 512}, ATGL_LAYOUT_VERTICAL,   0, 0);
    txt_cols_node = ATGL_CREATE_PANEL(MAIN_PANEL, (ATGL_RECT){32, 544, 512,  32}, ATGL_LAYOUT_HORIZONTAL, 0, 0);
    board_root    = ATGL_CREATE_PANEL(MAIN_PANEL, (ATGL_RECT){32,  32, 512, 512}, ATGL_LAYOUT_NONE,       0, 0);

    /* Right sidebar */
    RIGHT_PANEL = ATGL_CREATE_PANEL(MAIN_PANEL, (ATGL_RECT){560, 10, 220, 560}, ATGL_LAYOUT_NONE, 0, 0);

    RESET_BTN        = ATGL_CREATE_BUTTON(RIGHT_PANEL, (ATGL_RECT){  5,  5, 100, 36}, "Reset",       RESET_CALLBACK);
    SWITCH_MODE_BTN  = ATGL_CREATE_BUTTON(RIGHT_PANEL, (ATGL_RECT){110,  5, 104, 36}, "Switch Mode", SWITCH_MODE_CALLBACK);
    mode_label_node  = ATGL_CREATE_LABEL (RIGHT_PANEL, (ATGL_RECT){  5, 47, 210, 22}, "Mode: PvP",   VBE_BLACK, VBE_SEE_THROUGH);
    turn_label_node  = ATGL_CREATE_LABEL (RIGHT_PANEL, (ATGL_RECT){  5, 72, 210, 22}, "Turn: White", VBE_BLACK, VBE_SEE_THROUGH);
    status_label_node= ATGL_CREATE_LABEL (RIGHT_PANEL, (ATGL_RECT){  5, 97, 210, 22}, "",            VBE_RED,   VBE_SEE_THROUGH);

    ATGL_CREATE_LABEL(RIGHT_PANEL, (ATGL_RECT){5, 126, 210, 20}, "--- Move History ---", VBE_BLACK, VBE_SEE_THROUGH);
    for(int i=0;i<HISTORY_DISPLAY_ROWS;i++)
        history_labels[i]=ATGL_CREATE_LABEL(RIGHT_PANEL,(ATGL_RECT){5,150+i*18,210,18},"",VBE_BLACK,VBE_SEE_THROUGH);

    /* Build board */
    U8 buf[2]={0};
    for(U32 row=0;row<ROWS;row++){
        SNPRINTF(buf,sizeof(buf),"%d",ROWS-row);
        ATGL_CREATE_LABEL(num_rows_node,(ATGL_RECT){0,row*64,32,64},buf,VBE_BLACK,VBE_SEE_THROUGH);
        for(U32 col=0;col<COLS;col++){
            if(row==0){SNPRINTF(buf,sizeof(buf),"%c",'A'+col);ATGL_CREATE_LABEL(txt_cols_node,(ATGL_RECT){col*64,0,64,32},buf,VBE_BLACK,VBE_SEE_THROUGH);}
            ATGL_RECT rect={.x=col*64,.y=row*64,.w=64,.h=64};
            buf[0]=chessboard[row][col];buf[1]='\0';
            board_nodes[row][col]=ATGL_CREATE_BUTTON(board_root,rect,buf,MOVEMENT_CALLBACK);
        }
    }
    CLEAR_PIECE_MOVEMENT_TABLE();
    return 0;
}

/* ============================================================
   Event & Render loops
   ============================================================ */



VOID ATGL_EVENT_LOOP(ATGL_EVENT *ev) {
    if (ev->type == ATGL_EVT_KEY_DOWN && ev->key.keycode == KEY_ESC) {
        ATGL_QUIT();
        return;
    }
    
    ATGL_DISPATCH_EVENT(ATGL_GET_SCREEN_ROOT_NODE(), ev);
}

VOID ATGL_GRAPHICS_LOOP(U32 ticks) {
    ATGL_RENDER_TREE(ATGL_GET_SCREEN_ROOT_NODE());
}