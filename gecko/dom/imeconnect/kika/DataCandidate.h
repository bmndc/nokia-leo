#ifndef __IQQI_DATA_CANDIDATE_H
#define __IQQI_DATA_CANDIDATE_H

#include "WType.h"


//-----------------------------------------------------------------------------
//
//
//
//-----------------------------------------------------------------------------
struct CandidateCH
{
  IQWCHAR   **matrix;
  int       row, col;

  CandidateCH()
  {
    matrix = NULL;
  }
  CandidateCH(int rowN, int colN)
  {
    matrix = NULL;

    alloc(rowN, colN);
  }
  ~CandidateCH()
  {
    empty();
  }
  void empty()
  {
    if (matrix) {
      for (int i=0; i<row; i++) {
        delete matrix[i];
      }
      delete [] matrix;

      matrix = NULL;
    }
  }
  void alloc(int rowN, int colN)
  {
    if (matrix) {
      empty();
    }

    this->row = rowN + 1;
    this->col = colN + 1;

    matrix = new IQWCHAR*[row];

    for (int i=0; i<row; i++) {
      matrix[i] = new IQWCHAR[col];

      for ( int j=0; j<col; j++) {
        matrix[i][j] = 0;
      }
    }
  }
  bool isEmpty(int row)
  {
    if (row >= this->row) {
      return true;
    }

    return matrix[row][0] == 0;
  }
  int vlen(int row)
  {
    if (row >= this->row) {
      return 0;
    }

    return wcslen(matrix[row]);
  }
  IQWCHAR *voca(int row)
  {
    if (row >= this->row) {
      return 0;
    }

    return matrix[row];
  }
  const IQWCHAR *record(int row)
  {
    if (row >= this->row) {
      return 0;
    }

    return matrix[row];
  }
  operator IQWCHAR **()
  {
    return matrix;
  }
  IQWCHAR **pointer()
  {
    return matrix;
  }
};
typedef CandidateCH*  LPCandidateCH;


#endif
