#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/audio.c"
#else

#undef TAPI
#define TAPI __declspec(dllimport)

/* ---------------------------------------------------------------------- */
/* -- */
/* -- Copyright (c) 2012 Soumith Chintala */
/* --  */
/* -- Permission is hereby granted, free of charge, to any person obtaining */
/* -- a copy of this software and associated documentation files (the */
/* -- "Software"), to deal in the Software without restriction, including */
/* -- without limitation the rights to use, copy, modify, merge, publish, */
/* -- distribute, sublicense, and/or sell copies of the Software, and to */
/* -- permit persons to whom the Software is furnished to do so, subject to */
/* -- the following conditions: */
/* --  */
/* -- The above copyright notice and this permission notice shall be */
/* -- included in all copies or substantial portions of the Software. */
/* --  */
/* -- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, */
/* -- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF */
/* -- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND */
/* -- NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE */
/* -- LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION */
/* -- OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION */
/* -- WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */
/* --  */
/* ---------------------------------------------------------------------- */
/* -- description: */
/* --     audio.c - a wrapper from libSox to Torch-7 */
/* -- */
/* -- history:  */
/* --     May 26th, 2012, 5:46PM - wrote stft and channel 
                                  flattening functions - Soumith Chintala */
/* ---------------------------------------------------------------------- */


// write audio.toMono() which converts a multi-channel audio to single channel

static inline void audio_(apply_window)(double *input, 
                                       long window_size, int window_type) {
    /* switch (window_type) { */
    /* case 1: // Rectangular Window, do nothing */
    /*   break; */
    /* case 2: // Hamming Window */
    /*   audio_(apply_hamming)(buffer, window_size); */
    /*   break; */
    /* case 3: // Hann Window */
    /*   audio_(apply_hann)(buffer, window_size); */
    /*   break; */
    /* case 4: // Bartlett Window */
    /*   audio_(apply_bartlett)(buffer, window_size); */
    /*   break; */
    /* case 5: // Kaiser Window */
    /*   audio_(apply_kaiser)(buffer, window_size); */
    /*   break; */
    /* default: */
    /*   abort_("[stft_generic] Unknown window_type"); */
    /*   break; */
    /* } */
}

// generic short-time fourier transform function that supports multiple window types
// arguments [tensor, window-size, window-type, hop-size/stride]
// window_type [1, 2, 3, 4, 5] for [rectangular, hamming, hann, bartlett, kaiser]
static THTensor * audio_(stft_generic)(THTensor *input, 
                                       long window_size, int window_type, 
                                       long stride)
{
  if (input->size[0] > 1)
    abort_("[stft_generic] Multi-channel stft not supported");
  long length = input->size[1];
  real *input_data = THTensor_(data)(input);
  long nwindows = ((length - window_size)/stride) + 1;
  THTensor *output = THTensor_(newWithSize2d)(nwindows, window_size );
  real *output_data = THTensor_(data)(output);
  double *buffer = malloc(sizeof(double) * window_size);
  fftw_complex *fbuffer = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*window_size);
  long index, k;
  fftw_plan plan = fftw_plan_dft_r2c_1d(window_size, buffer, fbuffer, FFTW_ESTIMATE);

  // loop over the input. get a buffer. apply window. call stft. repeat with stride.
  for (index = 0; index + window_size <= length; index = index + stride) {
    for (k=0; k<window_size; k++)
      buffer[k] = (double)input_data[index+k];
    audio_(apply_window)(buffer, window_size, window_type);
    // now apply rfftw over the buffer
    fftw_execute(plan);
    for (k=0; k < window_size/2 + 1; k++)
      output_data[index + k] = (real)fbuffer[k][0];
  }

  // cleanup
  fftw_destroy_plan(plan);
  fftw_free(fbuffer);
  free(buffer);
  return output;
}

static int audio_(Main_stft)(lua_State *L) {
  THTensor *input = luaT_checkudata(L, 1, torch_(Tensor_id));
  long window_size = luaL_checklong(L, 2);
  int window_type = luaL_checkint(L, 3);
  long stride = luaL_checklong(L, 4);
  THTensor *output = audio_(stft_generic)(input, window_size, window_type, stride);
  luaT_pushudata(L, output, torch_(Tensor_id));
  return 1;
}

static const struct luaL_Reg audio_(Main__) [] = {
  {"stft", audio_(Main_stft)},
  {NULL, NULL}
};

void audio_(Main_init)(lua_State *L)
{
  luaT_pushmetaclass(L, torch_(Tensor_id));
  luaT_registeratname(L, audio_(Main__), "audio");
}

#endif
