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
#include "imbs.hpp"

using namespace cv;
using namespace boost::filesystem;


class IMBSBuilder : public IBGSAlgorithm
{
    
public:
    
    //default constructor
    IMBSBuilder();
    // parametric constructor
    IMBSBuilder(int, int, int);
    IMBSBuilder(Size, int);
    ~IMBSBuilder();
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

    //int           FramesToLearn;
    //int           SequenceLength;
    //int           TimeWindowSize;
    //unsigned char UpdateFlag;
    //unsigned char SDEstimationFlag;
    //unsigned char UseColorRatiosFlag;
    //double        Threshold;
    //double        Alpha;
    //unsigned int  InitFGMaskFrame;
    //unsigned int  EndFGMaskFrame;
    //unsigned char ApplyMorphologicalFilter;
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
    BackgroundSubtractorIMBS* model;

    double       fps;
    unsigned int fgThreshold;
    unsigned int associationThreshold;
    double       samplingPeriod;
    unsigned int minBinHeight;
    unsigned int numSamples;
    double       alpha;
    double       beta;
    double       tau_s;
    double       tau_h;
    double       minArea;
    double       persistencePeriod;
    bool         morphologicalFiltering;

    static const double       DefaultFps;
    static const unsigned int DefaultFgThreshold;
    static const unsigned int DefaultAssociationThreshold;
    static const double       DefaultSamplingPeriod;
    static const unsigned int DefaultMinBinHeight;
    static const unsigned int DefaultNumSamples;
    static const double       DefaultAlpha;
    static const double       DefaultBeta;
    static const double       DefaultTau_s;
    static const double       DefaultTau_h;
    static const double       DefaultMinArea;
    static const double       DefaultPersistencePeriod;
    static const bool         DefaultMorphologicalFiltering;

};


#endif
