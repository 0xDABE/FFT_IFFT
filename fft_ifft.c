#include "fft_ifft.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PI 3.1415926535897932385

#ifdef TABLE
#include "trig_table.h"

static inline number sinx(number arg) {
  size_t current_index = 0;
  char sign = 1;

  
  if(arg < 0) {
    sign = -1;
    arg = -arg;
  } 
  if((int)(arg / (2*PI)) != 0) {arg = arg - (number) ( (int) (arg/(2*PI)) ) *2*PI;}

  /* Преобразования, приводящие arg в промежуто от 0 до PI/2 */

  if((arg>=0) && (arg<=PI/2)) {
    current_index = (size_t) (arg/TABLESTEP);
    if(current_index == TABLESIZE - 1) return sign;
  }
  else if((arg>PI/2) && (arg<=PI)) {
    arg = PI - arg;
    current_index = (size_t) (arg/TABLESTEP);
    if(current_index == 0) return 0;
  }
  else if((arg>PI) && (arg<=3*PI/2)) {
    sign = -sign;
    arg = arg - PI;
    current_index = (size_t) (arg/TABLESTEP);
    if(current_index == TABLESIZE-1) return sign;
  }
  else if((arg>3*PI/2) && (arg<=2*PI)) {
    sign = -sign;
    arg = 2*PI - arg;
    current_index = (size_t) (arg/TABLESTEP);
    if(current_index == 0) return 0;
  }

  /* Аппроксимация промежутка между узлами таблицы линейной функцией */
  return (sin_table[current_index] + ((sin_table[current_index+1] - sin_table[current_index])*(arg-current_index*TABLESTEP))/(TABLESTEP))*sign;

}

/* Макрос для умножения комплексного числа в алгебраической форме на комплексное число в показательной */

#define EXPMULTBODY \
  complex temp = {0,0}; \
  temp.Real = arg->Real*sinx(*pow + PI/2) - arg->Imagine*sinx(*pow); \
  temp.Imagine = arg->Imagine*sinx(*pow + PI/2) + arg->Real*sinx(*pow); \
  return temp;
#endif

#ifndef TABLE
#include <math.h>

#define EXPMULTBODY \
  complex temp = {0,0}; \
  temp.Real = arg->Real*sin(*pow + PI/2) - arg->Imagine*sin(*pow); \
  temp.Imagine = arg->Imagine*sin(*pow + PI/2) + arg->Real*sin(*pow); \
  return temp;
#endif

/* Вспомогательная функция для умножения комплексного числа в алгебраической форме на комплексное число в показательной */

static inline complex expMult(const complex* arg, const number* pow) { EXPMULTBODY }

/* Вспомогательный макрос для умножения комплексных чисел в алгебраической форме */

#define COMPLEXMULT(out, op1, op2) \
  out.Real = (op1.Real)*(op2.Real) - (op1.Imagine)*(op2.Imagine); \
  out.Imagine = (op1.Real)*(op2.Imagine) + (op1.Imagine)*(op2.Real)

/* Вспомогательный макрос для сложения комплексных чисел в алгебраической форме */

#define COMPLEXADD(out, op1, op2) \
  out.Real = op1.Real + op2.Real; \
  out.Imagine = op1.Imagine + op2.Imagine



static int fft_rec(number* in_vector, complex* out_vector, size_t size, size_t initSize, size_t koefA, size_t koefB) {

  if(0 == size%2) {
    // Делим на две суммы с прореживанием по отсчетам

    // Заполнение первой суммы, четные отсчеты
    fft_rec(in_vector, out_vector, size/2, initSize, 2*koefA, koefB);

    // Заполнение второй суммы, нечетные отсчеты
    fft_rec(in_vector, (out_vector + size/2), size/2, initSize, 2*koefA, (koefA+koefB));

    // Прямое вычисление

    complex op1, op2, multResult;

    // Записываем результат в выходной вектор
    for(size_t i=0; i<(size/2); i++) {

      op1 = out_vector[i];
      op2 = out_vector[i + (size/2)];

      // вычисляем i-й элемент выходного вектора
      number pow = -(2*PI*i)/(size);
      multResult = expMult(&op2, &pow);
      COMPLEXADD(out_vector[i], op1, multResult);

      // вычисляем (i + size/2)-й элемент выходно вектора

      pow = -(2*PI*(i+((number) size/2)))/(size);
      multResult = expMult(&op2, &pow);
      COMPLEXADD(out_vector[i+(size/2)], op1, multResult);
    }

  }

  // прореживание невозможно, вычисляем прямо

  else {

    complex multResult;

    for(size_t i=0; i<size; i++) {

      out_vector[i].Real = 0;
      out_vector[i].Imagine = 0;

      for(size_t j=0; (j*koefA + koefB)<initSize; j++) {

        number pow = -(2*PI*i*j)/(size);
        complex current = {in_vector[(j*koefA + koefB)], 0};
        multResult = expMult(&current, &pow);

        COMPLEXADD(out_vector[i], out_vector[i], multResult);
      }

    }

  }

  return 0;
}

int dft(number* in_vector, complex* out_vector, size_t size) {

  complex multResult;

  for(size_t i=0; i<size; i++) {

    out_vector[i].Real = 0;
    out_vector[i].Imagine = 0;

    for(size_t j=0; j<size; j++) {

      number pow = -(2*PI*i*j)/(size);
      complex current = {in_vector[j], 0};
      multResult = expMult(&current, &pow);

      COMPLEXADD(out_vector[i], out_vector[i], multResult);
    }

  }
  return 0;
}

int fft(number* in_vector, complex* out_vector, size_t size) {
  return fft_rec(in_vector, out_vector, size, size, 1, 0);
}

static int ifft_rec(complex* in_vector, complex* out_vector, size_t size, size_t initSize, size_t koefA, size_t koefB) {

  if(0 == size%2) {
    // Делим на две суммы с прореживанием по отсчетам

    // Заполнение первой суммы, четные отсчеты
    ifft_rec(in_vector, out_vector, size/2, initSize, 2*koefA, koefB);

    // Заполнение второй суммы, нечетные отсчеты
    ifft_rec(in_vector, (out_vector + size/2), size/2, initSize, 2*koefA, (koefA+koefB));

    // Прямое вычисление

    complex op1, op2, multResult;

    // Записываем результат в выходной вектор
    for(size_t i=0; i<(size/2); i++) {

      op1 = out_vector[i];
      op2 = out_vector[i + (size/2)];

      // вычисляем i-й элемент выходного вектора
      number pow = (2*PI*i)/(size);
      multResult = expMult(&op2, &pow);
      COMPLEXADD(out_vector[i], op1, multResult);

      // вычисляем (i + size/2)-й элемент выходно вектора

      pow = (2*PI*(i+((number) size/2)))/(size);
      multResult = expMult(&op2, &pow);
      COMPLEXADD(out_vector[i+(size/2)], op1, multResult);
    }

  }

  // прореживание невозможно, вычисляем прямо

  else {

    complex multResult;

    for(size_t i=0; i<size; i++) {

      out_vector[i].Real = 0;
      out_vector[i].Imagine = 0;

      for(size_t j=0; (j*koefA + koefB)<initSize; j++) {

        number pow = (2*PI*i*j)/(size);
        complex current = in_vector[(j*koefA + koefB)];
        multResult = expMult(&current, &pow);

        COMPLEXADD(out_vector[i], out_vector[i], multResult);
      }

    }

  }

  if(size == initSize) {
    for(size_t i=0; i<size; i++) {
      out_vector[i].Real = ((number)1/size) * out_vector[i].Real;
      out_vector[i].Imagine = ((number)1/size) * out_vector[i].Imagine;
    }
  }

  return 0;

}

int ifft(complex* in_vector, complex* out_vector, size_t size) {
  return ifft_rec(in_vector, out_vector, size, size, 1, 0);
}
