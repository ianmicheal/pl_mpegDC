#ifndef __MATRIX_H__
#define __MATRIX_H__

extern void clear_matrix();
extern void apply_matrix(float matrix[4][4]);
extern void transform_coords(float src[][3], float dest[][4], int n);

#endif
