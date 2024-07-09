/*
 * chessgame.C
 *
 * PostgreSQL Complex Number Type:
 *
 * complex '(a,b)'
 *
 * Author: Maxime Schoemans <maxime.schoemans@ulb.be>
 */

#include <smallchesslib.h>
#include <stdio.h>
#include <postgres.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "utils/builtins.h"
#include "libpq/pqformat.h"

PG_MODULE_MAGIC;

#define EPSILON         1.0E-06

#define FPzero(A)       (fabs(A) <= EPSILON)
#define FPeq(A,B)       (fabs((A) - (B)) <= EPSILON)
#define FPne(A,B)       (fabs((A) - (B)) > EPSILON)
#define FPlt(A,B)       ((B) - (A) > EPSILON)
#define FPle(A,B)       ((A) - (B) <= EPSILON)
#define FPgt(A,B)       ((A) - (B) > EPSILON)
#define FPge(A,B)       ((B) - (A) <= EPSILON)

/*****************************************************************************/

/* Structure to represent complex numbers */
typedef struct
{
  double    a,
            b;
} Complex;

/* fmgr macros ChessGame type */

#define DatumGetChessGameP(X)  ((SCL_Record *) DatumGetPointer(X))
#define DatumGetChessBoardP(X) ((SCL_Board *) DatumGetPointer(X))
#define ChessBoardPGetDatum(X) PointerGetDatum(X)
#define ChessGamePGetDatum(X)  PointerGetDatum(X)
#define PG_GETARG_ChessGame_P(n) DatumGetChessGameP(PG_GETARG_DATUM(n))
#define PG_GETARG_ChessBoard_P(n) DatumGetChessBoardP(PG_GETARG_DATUM(n))
#define PG_RETURN_ChessGame_P(x) return ChessGamePGetDatum(x)
#define PG_RETURN_ChessBoard_P(x) return ChessBoardPGetDatum(x)

/*****************************************************************************/

static SCL_Record *
Chessgame_make(char *str)
{
  SCL_Record *c = palloc0(SCL_RECORD_MAX_LENGTH);
//  SCL_Record *c = palloc0(SCL_RECORD_MAX_LENGTH);
  SCL_recordFromPGN(*c,str);
  //SCL_recordFromPGN(*c,"1. f3 e5 2. g4 Qh4#");
  //SCL_recordFromPGN(*c,"1. Qxc4 c6 2. g4 Qh4#i");
//  SCL_recordFromPGN(*c,"");
  return *c;


}

static SCL_Board *
Chessboard_make(char *str)
{
  SCL_Board *b = palloc0(SCL_BOARD_STATE_SIZE);
  SCL_boardInit(*b);
  SCL_boardFromFEN(*b,str);


  /*char *output = palloc0(SCL_FEN_MAX_LENGTH);
  SCL_boardToFEN(b,output);
  ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
  errmsg("output: %s",output)));*/

  //SCL_boardFromFEN(*b,'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1');
  //SCL_boardFromFEN(*b,'8/5k2/3p4/1p1Pp2p/pP2Pp1P/P4P1K/8/8 b - - 99 50');
//  SCL_boardFromFEN(*b,'r4r1k/ppp1qpb1/7p/4pRp1/1PB1P3/1QPR3P/P4P2/6K1 w - - 4 26');
  return b;

}
/*****************************************************************************/

static void
p_whitespace(char **str)
{
   while (**str == ' ' || **str == '\n' || **str == '\r' || **str == '\t')
      *str += 1;
}

static void
ensure_end_input(char **str, bool end)
{
  *str += 1;
  if (end)
  {
    p_whitespace(str);
    if (**str != 0)
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Could not parse temporal value %c",**str)));

  }
}

static int
p_turnNumber(char **str,int numberOfTurns)
{
  p_whitespace(str);
  /*int result = **str - '0';
  *str += 1;
  return result == numberOfTurns;*/
  if( (**str - '0' )==  numberOfTurns )  {
    *str += 1;
    return true;
  }
  else
    return false;
}

static bool
p_dot(char **str)
{
p_whitespace(str);
  if (**str == '.')
  {
    *str += 1;
    p_whitespace(str);
   return true;
  }
  return false;
}


static bool
is_uppercase_letter(char **str)
{
  if  (**str == 'Q' || **str == 'K' || **str == 'R' || **str == 'B' ||
		  **str == 'N' )
      return true;
  return false;
}
static bool
is_lowercase_letter(char **str)  {
  if (**str == 'a' || **str == 'b' || **str == 'c' || **str == 'd' ||
     **str == 'e' || **str == 'f' || **str == 'g' || **str == 'h' )
      return true;
  return false;
}

static bool
is_number(char **str)  {
  if (**str == '1' || **str == '2' || **str == '3' || **str == '4' ||
     **str == '5' || **str == '6' || **str == '7' || **str == '8' )
      return true;
  return false;
}

static bool
is_move(char **str)
{
  if (is_lowercase_letter(str))  {
    *str+= 1;
    if (is_number(str))  {
        *str -= 1;
        return true;
    }
    *str -= 1;
  }
  return false;
}

static bool
is_firstChar(char **str)
{
  bool bool1;
  *str -= 1;
  bool1 = **str == ' ';
  *str +=1;
  return bool1;

}

static bool
p_uppercase_letter(char **str)
{
  /*bool bool1, bool2;
  bool1 = is_firstChar(str);
  *str -= 2;
  bool2 = is_move(str);
  *str += 3;
  return bool1 || bool2;*/

  if (is_firstChar(str))  {
    *str += 1;
    return true;
  }

  else {
    *str -= 2;
    if (is_move(str))  {
      *str += 2;
       return true;
    }
  }
  return false;
}

static bool
p_lowercase_letter(char **str)
{
  if (is_move(str))  {
    *str += 2;
     //if (**str == '#')  {
     // ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
      // errmsg("Invvalid input syntax at the line 200 with at the char %c",**str)));
     // }
    return true;
  }
  else  {
//     if (**str == 'f')  {
  //    ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
   //    errmsg("Invvalid input syntax at the line 200 with at the char %c",**str)));
    //  }
    bool bool1, bool2, bool3;
    **str -= 1;
    bool1 = is_uppercase_letter(str);
    *str += 2;
    bool2 = is_lowercase_letter(str) ;
    bool3 = **str == 'x';
    //*str += 1;
    return bool2 || bool3;
   // return bool1 && (bool2 || bool3);
  }
}

static bool
p_number(char **str)
{
  bool bool1,bool2;
  *str -= 1;
  bool1 = is_lowercase_letter(str);
  bool2 = is_uppercase_letter(str);
  *str += 2;
  return bool1 || bool2;

}

static bool
p_x(char **str)
{
  *str += 1;
  ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
  errmsg("Invvalid input syntax for type Chessgame with the character %c, at the line 237",**str)));
  if (is_move(str))  {
    *str += 2;
    return true;
  }
  else
    return false;
}

static bool
p_check(char **str,char *firstChar)
{
  bool result;
  *str += 1;
  result = **str == ' ' && *firstChar == 'K';
  return result;
}

static bool
is_castling(char** str)
{
  bool bool1,bool2;
  *str += 1;
  bool1 = **str == '-';
  *str += 1;
  bool2 = **str == 'O';
  return bool1 && bool2;
}

static bool
p_castling(char **str)
{
  if (is_castling(str))  {  //kingside
    *str += 1;
    return true;
  }
  else {
    bool bool1 = is_castling(str);  //queesnide
    *str += 1;
    return bool1;

  }
  //return kingside(str)  || queenside(str);
}
static bool
p_move(char **str,bool whitePlay,char *firstChar,int numberOfTurns)
{
   p_whitespace(str);
   char *move = palloc(sizeof(char)*4);
   while (**str != ' ' )  {
     bool test;
     //if (**str == 'c')  {
     // ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
     //  errmsg("Invvalid input syntax at the line 294 with at the char %c",**str)));
     // }
     if (is_uppercase_letter(str))  {
       if (is_firstChar(str)  && **str == 'K')
         *firstChar = 'K';
       test = p_uppercase_letter(str);
     }
     else if  (is_lowercase_letter(str))  {
      // if (**str == 'c')  {
      //   ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        // errmsg("Invvalid input syntax at the line 200 with at the char %c",**str)));
      // }
       test = p_lowercase_letter(str);
     }
     else if (is_number(str))
       test = p_number(str);
     else if (**str == 'x')
       test = p_x(str);
     else if (**str == '+')
       test = p_check(str,firstChar);
     else if (**str == 'O')
       test = p_castling(str);
     else
       test = false;
     if (!test)
       return false;
     if (**str == '#')
       break;
    }
      //ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
       // errmsg("Invvalid input syntax at the line 294 with at the string %s",move)));
    return true;

}

static SCL_Record *
Chessgame_parse(char **str)
{
  return Chessgame_make(*str);
  int numberOfTurns = 0;
  while (**str != '#')  {
    numberOfTurns += 1;
    if ( !p_turnNumber(str,numberOfTurns))
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Invalid input syntax for type Chessgame at the turn identifier number %d %c",numberOfTurns,**str)));
    if ( !p_dot(str))
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Invalid input syntax for type Chessgame with the dot at turn number %d",numberOfTurns)));
    char *firstChar = palloc(sizeof(char));
    if ( !p_move(str,true,firstChar,numberOfTurns))
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Invvalid input syntax for type Chessgame with the white play at turn number %d",numberOfTurns)));
    if (**str == '#')
      break;
    if ( !p_move(str,false,firstChar,numberOfTurns))
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Inval@id input syntax for type Chessgame with the black play at turn number %d",numberOfTurns)));
    if (**str == '#')
      break;
  }
  ensure_end_input(str,true);
  
  return Chessgame_make(*str);

}

static char* ChessgameToStr(SCL_Record  *c){
  char *move = palloc0(sizeof(char)*3000);
  int numberOfTurns = 1;

  uint8_t source, destination;//, s2, s3
  for( int i=0; i< SCL_recordLength(*c);i++){
    if (i%2 == 0)  {
      char *turn = psprintf("%d. ",numberOfTurns);
      strcat(move,turn);
      numberOfTurns ++;
    }
    uint8_t source, destination;//, s2, s3
    char promotion;// p2
    SCL_recordGetMove(*c,i,&source,&destination,&promotion);
    
    char sourceString[10] = "";
    char destinationString[10] = "";
    SCL_squareToString(source,sourceString);
    strcat(move,sourceString);
    SCL_squareToString(destination,destinationString);
    strcat(move,destinationString);

    /*if (strcmp(sourceString,destinationString) == 0) {
      strcat(move,destinationString);
    }
    else   {
       strcat(move,sourceString);
       strcat(move,destinationString);
    }*/
    if (promotion != 'q')
      strncat(move,promotion,1);
    if (i+1 < SCL_recordLength(*c))
      strcat(move," ");
    else
      strcat(move,"#");
  }
    
  return move;
}

static bool compareOpening(SCL_Record *c,SCL_Record *d){
  for( int i=0; i< SCL_recordLength(*d);i++){
    uint8_t s0, s1,s2,s3;
    char p1,p2;
    SCL_recordGetMove(*c,i,&s0,&s1,&p1);
    SCL_recordGetMove(*d,i,&s2,&s3,&p2);
    if( s0 != s2 || s1 != s3 || p1 != p2){
      return 0;
    }
  }
  return 1;
}

static SCL_Record *getFirstMovesProcess(SCL_Record *chessgame, int halfmoves){
 /* 
  SCL_Record *newRecord = palloc0(SCL_RECORD_MAX_LENGTH);
  char test[300];
  SCL_recordInit(newRecord);
  uint16_t recordLength =  SCL_recordLength(chessgame);
  uint8_t squareFrom;
  uint8_t squareTo;
  uint8_t move;
  for(int i = 0 ; i < halfmoves ; i++){
    char promotedPiece;
    move = SCL_recordGetMove(*chessgame,i,&squareFrom,&squareTo,&promotedPiece);
    test[i] = promotedPiece;
    SCL_recordAdd(*newRecord,squareFrom,squareTo,promotedPiece,move);
    
  }

//  ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
//  errmsg("squareForm squareTo: %d %d",squareFrom,squareTo)));
  return newRecord;
  */
  int size = SCL_recordLength(*chessgame);
  int number = size - halfmoves;
  for(int i = 0; i < number; i++){
    SCL_recordRemoveLast(*chessgame);
  }
  return *chessgame;
}

static SCL_Board *
Chessboard_parse(char **str)
{
  return Chessboard_make(*str);
  /*
  Parsing
  int numberOfTurns = 0;
  while (**str != '#')  {
    numberOfTurns += 1;
    if ( !p_turnNumber(str,numberOfTurns))
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Invalid input syntax for type Chessgame at the turn identifier number %d %c",numberOfTurns,**str)));
    if ( !p_dot(str))
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Invalid input syntax for type Chessgame with the dot at turn number %d",numberOfTurns)));
    char *firstChar = palloc(sizeof(char));
    if ( !p_move(str,true,firstChar,numberOfTurns))
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Invvalid input syntax for type Chessgame with the white play at turn number %d",numberOfTurns)));
    if (**str == '#')
      break;
    if ( !p_move(str,false,firstChar,numberOfTurns))
      ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
        errmsg("Inval@id input syntax for type Chessgame with the black play at turn number %d",numberOfTurns)));
    if (**str == '#')
      break;
  }
  ensure_end_input(str,true);
  return Chessgame_make(*str);
 */
}

static char* ChessboardToStr(SCL_Board  *b){
  char *output = palloc0(SCL_FEN_MAX_LENGTH);
  uint8_t result = SCL_boardToFEN(b,output);
  if (result != 0)
    return output;
  else  {
    ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
    errmsg("Error could not charge the board %s",output)));
  }
}

static bool compareBoard(SCL_Board *boardFromRecord, SCL_Board *boardToCompare){
  return !SCL_boardsDiffer(*boardFromRecord,*boardToCompare);
}



/*****************************************************************************/

PG_FUNCTION_INFO_V1(chessgame_in);
Datum
chessgame_in(PG_FUNCTION_ARGS)
{
  char *str = PG_GETARG_CSTRING(0);
  //SCL_Record c;
  //SCL_recordInit(c);
  //SCL_recordFromPGN(c,str);
  //PG_RETURN_ChessGame_P(Chessgame_make(c));

  //PG_RETURN_ChessGame_P(Chessgame_make(str));
  PG_RETURN_ChessGame_P(Chessgame_parse(&str));
}

PG_FUNCTION_INFO_V1(chessgame_out);
Datum
chessgame_out(PG_FUNCTION_ARGS)
{
  SCL_Record *c = PG_GETARG_ChessGame_P(0);
  char* result = ChessgameToStr(c);
  PG_FREE_IF_COPY(c, 0);
  PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(complex_recv);
Datum
complex_recv(PG_FUNCTION_ARGS)
{
  StringInfo  buf = (StringInfo) PG_GETARG_POINTER(0);
  Complex *c = (Complex *) palloc(sizeof(Complex));
  c->a = pq_getmsgfloat8(buf);
  c->b = pq_getmsgfloat8(buf);
  PG_RETURN_ChessGame_P(c);
}

PG_FUNCTION_INFO_V1(hasOpening);
Datum
hasOpening(PG_FUNCTION_ARGS)
{
	SCL_Record *c = PG_GETARG_ChessGame_P(0);
	SCL_Record *d = PG_GETARG_ChessGame_P(1);
  PG_RETURN_BOOL(compareOpening(c,d));
}


PG_FUNCTION_INFO_V1(getFirstMoves);
Datum
getFirstMoves(PG_FUNCTION_ARGS)
{

  SCL_Record *chessGameRecord = PG_GETARG_ChessGame_P(0);
  int halfmoves = PG_GETARG_INT32(1);
  SCL_Record *c = getFirstMovesProcess(chessGameRecord,halfmoves);
  PG_RETURN_ChessGame_P(c);
}

PG_FUNCTION_INFO_V1(hasBoard);
Datum
hasBoard(PG_FUNCTION_ARGS)
{
  SCL_Record *chessGameRecord = PG_GETARG_ChessGame_P(0);
  SCL_Board *boardToCompare = PG_GETARG_ChessBoard_P(1);
  int halfmoves = PG_GETARG_INT32(2);
  SCL_Board *boardFromRecord = palloc0(SCL_BOARD_STATE_SIZE);
  int i = 1;
  bool result;
  for (i = 1; i <= halfmoves;i++)  {
    SCL_boardInit(*boardFromRecord);
    SCL_recordApply(chessGameRecord,boardFromRecord,i);
    result = compareBoard(boardFromRecord,boardToCompare);  
    if (result)  {
      break;
    }
  }
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(chessboard_in);
Datum
chessboard_in(PG_FUNCTION_ARGS)
{
  char *str = PG_GETARG_CSTRING(0);
  PG_RETURN_ChessBoard_P(Chessboard_parse(&str));

/*  SCL_Board b;
  SCL_boardInit(b);
  SCL_boardFromFEN(b,str);
  char *output = palloc0(SCL_FEN_MAX_LENGTH);
  SCL_boardToFEN(b,output);

  //ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
  //errmsg("output: %s",output)));
  */
 
}

PG_FUNCTION_INFO_V1(chessboard_out);
Datum
chessboard_out(PG_FUNCTION_ARGS)
{
  SCL_Board *b = (SCL_Board*)PG_GETARG_ChessBoard_P(0);
  SCL_boardEstimatePhase(b);
  char* result = ChessboardToStr(b);


 /* char *output = palloc0(SCL_FEN_MAX_LENGTH);
  SCL_boardToFEN(b,output);
  ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
  errmsg("b output: %s",output))); */
/* if (result == NULL) {
    ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                    errmsg("ChessboardToStr returned NULL")));
} else {
    printf("Result: %s\n", result); // Vérification préalable
    ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                    errmsg("b output: %s", result)));
}*/

  PG_FREE_IF_COPY(b, 0);
  PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(getBoard);
Datum
getBoard(PG_FUNCTION_ARGS)
{
  SCL_Record *chessGameRecord = PG_GETARG_ChessGame_P(0);
  int halfmoves = PG_GETARG_INT32(1);
  SCL_Board *boardFromRecord = palloc0(SCL_BOARD_STATE_SIZE);
  SCL_boardInit(*boardFromRecord);
  SCL_recordApply(chessGameRecord,boardFromRecord,halfmoves);
  /*ereport(ERROR, (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                    errmsg("b output:: ")));*/
  PG_RETURN_ChessBoard_P(boardFromRecord);
}

/*---------------------------------*/

chessgame_abs_cmp_internal(SCL_Record *a, SCL_Record *b)
{
  int r1 = SCL_recordLength(*a);
  int r2 = SCL_recordLength(*b);
  int n; 
  if(r1 > r2){
    n = r2;
  }else{
    n = r1;
  }
    uint8_t squareFrom1;
    uint8_t squareTo1;
    uint8_t squareFrom2;
    uint8_t squareTo2;
    for (int i = 0; i < n; i++)  {
      char promotedPiece1;
      char promotedPiece2;
      uint8_t move1 = SCL_recordGetMove(*a,i,&squareFrom1,&squareTo1,&promotedPiece1);
      uint8_t move2 = SCL_recordGetMove(*b,i,&squareFrom2,&squareTo2,&promotedPiece2);
      
    if (strcmp(squareFrom1,squareFrom2) < 0 )
      return -1;
        
    if (strcmp(squareFrom1,squareFrom2) > 0 )
        return 1;

    
    if (strcmp(squareTo1,squareTo2) < 0 )
        return -1;
    if (strcmp(squareTo1,squareTo2) > 0)
        return 1;
    }
    if(r1 > r2)return 1;
    if(r1 < r2) return -1;
    return 0;
}

PG_FUNCTION_INFO_V1(chessgame_abs_eq);
Datum
chessgame_abs_eq(PG_FUNCTION_ARGS)
{
  SCL_Record *c = PG_GETARG_ChessGame_P(0);
  SCL_Record *d = PG_GETARG_ChessGame_P(1);
  bool result = chessgame_abs_cmp_internal(c, d) == 0;
  PG_FREE_IF_COPY(c, 0);
  PG_FREE_IF_COPY(d, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(chessgame_abs_ne);
Datum
chessgame_abs_ne(PG_FUNCTION_ARGS)
{
  SCL_Record *c = PG_GETARG_ChessGame_P(0);
  SCL_Record *d = PG_GETARG_ChessGame_P(1);
  bool result = chessgame_abs_cmp_internal(c, d) != 0;
  PG_FREE_IF_COPY(c, 0);
  PG_FREE_IF_COPY(d, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(chessgame_abs_lt);
Datum
chessgame_abs_lt(PG_FUNCTION_ARGS)
{
  SCL_Record *c = PG_GETARG_ChessGame_P(0);
  SCL_Record *d = PG_GETARG_ChessGame_P(1);
  bool result = chessgame_abs_cmp_internal(c, d) < 0;
  PG_FREE_IF_COPY(c, 0);
  PG_FREE_IF_COPY(d, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(chessgame_abs_le);
Datum
chessgame_abs_le(PG_FUNCTION_ARGS)
{
  SCL_Record *c = PG_GETARG_ChessGame_P(0);
  SCL_Record *d = PG_GETARG_ChessGame_P(1);
  bool result = chessgame_abs_cmp_internal(c, d) <= 0;
  PG_FREE_IF_COPY(c, 0);
  PG_FREE_IF_COPY(d, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(chessgame_abs_gt);
Datum
chessgame_abs_gt(PG_FUNCTION_ARGS)
{
  SCL_Record *c = PG_GETARG_ChessGame_P(0);
  SCL_Record *d = PG_GETARG_ChessGame_P(1);
  bool result = chessgame_abs_cmp_internal(c, d) > 0;
  PG_FREE_IF_COPY(c, 0);
  PG_FREE_IF_COPY(d, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(chessgame_abs_ge);
Datum
chessgame_abs_ge(PG_FUNCTION_ARGS)
{
  SCL_Record *c = PG_GETARG_ChessGame_P(0);
  SCL_Record *d = PG_GETARG_ChessGame_P(1);
  bool result = chessgame_abs_cmp_internal(c, d) >= 0;
  PG_FREE_IF_COPY(c, 0);
  PG_FREE_IF_COPY(d, 1);
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(chessgame_abs_cmp);
Datum
chessgame_abs_cmp(PG_FUNCTION_ARGS)
{
  SCL_Record *c = PG_GETARG_ChessGame_P(0);
  SCL_Record *d = PG_GETARG_ChessGame_P(1);
  int result = chessgame_abs_cmp_internal(c, d);
  PG_FREE_IF_COPY(c, 0);
  PG_FREE_IF_COPY(d, 1);
  PG_RETURN_INT32(result);
}




