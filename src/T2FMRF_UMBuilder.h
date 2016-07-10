/*******************************************************************************
 * This file is part of libraries to evaluate performance of Background 
 * Subtraction algorithms.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef _IMBS_ALGORITHM_H
#define _IMBS_ALGORITHM_H

#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>
#include "IBGSAlgorithm.h"
#include "T2FMRF.h"
#include "MRF.h"

using namespace cv;
using namespace boost::filesystem;
using namespace Algorithms::BackgroundSubtraction;


class T2FMRF_UMBuilder: public IBGSAlgorithm
{
    
public:
    
    //default constructor
    T2FMRF_UMBuilder();
    // parametric constructor
    T2FMRF_UMBuilder(int, int, int);
    T2FMRF_UMBuilder(Size, int);
    ~T2FMRF_UMBuilder();
    void SetAlgorithmParameters()  {};
    void LoadConfigParameters();
    void SaveConfigParameters() {};
    void Initialization();
    void GetBackground(OutputArray);
    void GetForeground(OutputArray);
    void Update(InputArray, OutputArray);
    void LoadModel() {};
    void SaveModel() {};
    string PrintParameters();
    const string Name() {return AlgorithmName; };
    string ElapsedTimeAsString();
    double ElapsedTime(){ return duration; };

private:
    void loadDefaultParameters();

    int rows;
    int cols;
    int nchannels;
    bool has_been_initialized;
    int frame_counter;
    Size frameSize;
    int  frameType;
    double duration;

    //unsigned char *FilterFGImage;
    static const string AlgorithmName;


    /*
     * Below are declarations of parameter specific for the algorithm.
     */
    MRF_TC mrf;
    GMM *gmm;
    HMM *hmm;
    T2FMRFParams modelParams;
    T2FMRF* model;

    BwImage lowThresholdMask;
    BwImage highThresholdMask;

    IplImage* input_frame;
    RgbImage  model_frame;
    IplImage* previous_frame;
    IplImage* previous_saved;

    long         frameNumber;
    double       threshold;
    double       alpha;
    float        km;
    float        kv;
    int          gaussians;

    static const long         DefaultFrameNumber;
    static const double       DefaultThreshold;
    static const double       DefaultAlpha;
    static const float        DefaultKm;
    static const float        DefaultKv;
    static const int          DefaultGaussians;

};


#endif
