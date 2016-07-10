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
#include "T2FMRF_UMBuilder.h"


const string       T2FMRF_UMBuilder::AlgorithmName      = "T2FMRF_UM";
const long         T2FMRF_UMBuilder::DefaultFrameNumber = 0;
const double       T2FMRF_UMBuilder::DefaultThreshold   = 9.; 
const double       T2FMRF_UMBuilder::DefaultAlpha       = 0.001;
const float        T2FMRF_UMBuilder::DefaultKm          = 1.5f;
const float        T2FMRF_UMBuilder::DefaultKv          = 0.6f; 
const int          T2FMRF_UMBuilder::DefaultGaussians   = 3;


T2FMRF_UMBuilder::T2FMRF_UMBuilder()
{
    loadDefaultParameters();
    cols = 0;
    rows = 0;
    nchannels = 0;
    has_been_initialized = false;
    frame_counter = 0;
    model_frame.ReleaseMemory(false);

}

T2FMRF_UMBuilder::T2FMRF_UMBuilder(int _col, int _row, int _nchannels)
{
    loadDefaultParameters();
    cols = _col;
    rows = _row;
    nchannels = _nchannels;
    has_been_initialized = false;
    frame_counter = 0;
    model_frame.ReleaseMemory(false);

}

T2FMRF_UMBuilder::T2FMRF_UMBuilder(Size _size, int _type)
{
    loadDefaultParameters();
    frameSize = _size;
    frameType = _type;
    rows = frameSize.height;
    cols = frameSize.width;
    nchannels = CV_MAT_CN(frameType);
    has_been_initialized = false;
    frame_counter = 0;
    model_frame.ReleaseMemory(false);
}


T2FMRF_UMBuilder::~T2FMRF_UMBuilder()
{
    if (model != NULL) {
        delete model;
    }
    model = 0;
}

void T2FMRF_UMBuilder::Initialization()
{

    if (rows != 0 && cols != 0 && nchannels != 0) {
        lowThresholdMask = cvCreateImage(cvSize(cols, rows), IPL_DEPTH_8U, 1);
        lowThresholdMask.Ptr()->origin = IPL_ORIGIN_BL;

        highThresholdMask = cvCreateImage(cvSize(cols, rows), IPL_DEPTH_8U, 1);
        highThresholdMask.Ptr()->origin = IPL_ORIGIN_BL;

        modelParams.SetFrameSize(cols, rows);
        modelParams.LowThreshold()  = threshold;
        modelParams.HighThreshold() = 2*modelParams.LowThreshold();
        modelParams.Alpha()         = alpha;
        modelParams.MaxModes()      = gaussians;
        modelParams.Type()          = TYPE_T2FMRF_UM;
        modelParams.KM()            = km; // Factor control for the T2FMRF-UM [0,3] default: 1.5
        modelParams.KV()            = kv; // Factor control for the T2FMRF-UV [0.3,1] default: 0.6


        model = new T2FMRF();
        model->Initalize(modelParams);
        
        RgbImage dummy = cvCreateImage(cvSize(cols, rows), IPL_DEPTH_8U, 1);
        model->InitModel(dummy);


        previous_frame = cvCreateImage(cvSize(cols, rows), IPL_DEPTH_8U, 1);
        previous_saved = cvCreateImage(cvSize(cols, rows), IPL_DEPTH_8U, 1);

        mrf.height = rows;
        mrf.width  = cols;
        mrf.Build_Classes_OldLabeling_InImage_LocalEnergy();

        has_been_initialized = true;
    }

    
}

void T2FMRF_UMBuilder::GetBackground(OutputArray image) 
{
    /*
    Mat img_background;
    model->getBackgroundImage(img_background);
    img_background.copyTo(image);
    */
}

void T2FMRF_UMBuilder::GetForeground(OutputArray mask) 
{

}

void T2FMRF_UMBuilder::Update(InputArray frame, OutputArray mask) 
{

    Mat Image = frame.getMat();
    //Mat Foreground(Image.size(),CV_8U,Scalar::all(0));

    input_frame = new IplImage(Image);
    model_frame = input_frame;


    model->Subtract(frame_counter , model_frame, lowThresholdMask, highThresholdMask);

    cvCopy(lowThresholdMask.Ptr(), previous_frame);

    if(frame_counter >=10) 
    {
        gmm = model->gmm();
        hmm = model->hmm();
        mrf.background2 = model_frame.Ptr();
        mrf.in_image    = lowThresholdMask.Ptr();
        mrf.out_image   = lowThresholdMask.Ptr();
        mrf.InitEvidence2(gmm,hmm,previous_saved);
        mrf.ICM2();
        cvCopy(mrf.out_image, lowThresholdMask.Ptr());
    }

    cvCopy(previous_frame, previous_saved);

    lowThresholdMask.Clear();
    model->Update(frame_counter, model_frame, lowThresholdMask);

    Mat Foreground(highThresholdMask.Ptr());

    Foreground.copyTo(mask);
    delete input_frame;

    frame_counter += 1;




    /*
    Mat Image = frame.getMat();
    Mat Foreground(Image.size(),CV_8U,Scalar::all(0));

        
    // Check model has not been initialized.
    if ( !has_been_initialized ) {
        Size frameSize = Image.size();
        int  frameType = Image.type();
        rows = frameSize.height;
        cols = frameSize.width;
        nchannels = CV_MAT_CN(frameType);
        Initialization();
    }

    //get the fgmask and update the background model
    model->apply(Image, Foreground);
    Foreground.copyTo(mask);
    frame_counter += 1;
    */
}


void T2FMRF_UMBuilder::loadDefaultParameters()
{

    frameNumber = DefaultFrameNumber;
    threshold   = DefaultThreshold  ;
    alpha       = DefaultAlpha      ;
    km          = DefaultKm         ;
    kv          = DefaultKv         ; 
    gaussians   = DefaultGaussians  ;
}

string T2FMRF_UMBuilder::PrintParameters()
{
    std::stringstream str;
    str
    << "# " 
    //<< "FrameNumber="  << frameNumber << " "
    << "Threshold="    << threshold   << " "
    << "Alpha="        << alpha       << " "
    << "Km="           << km          << " "
    << "Kv="           << kv          << " " 
    << "Gaussians="    << gaussians   ;
    return str.str();

}

void T2FMRF_UMBuilder::LoadConfigParameters()
{
    string nameLowercase(AlgorithmName);
    std::transform(AlgorithmName.begin(),
                   AlgorithmName.end(),
                   nameLowercase.begin(),
                   ::tolower);

    string filename = "config/" + nameLowercase + ".xml";

    if ( exists(filename) && is_regular_file(filename) ) {
        
        FileStorage fs(filename, FileStorage::READ);

        frameNumber = (int)fs["FrameNumber"];
        threshold   = (double)fs["Threshold"];
        alpha       = (double)fs["Alpha"];
        km          = (float)fs["Km"];
        kv          = (float)fs["Kv"];
        gaussians   = (int)fs["Gaussians"];

        fs.release();
    }
    else {
        path p("config");
        if (!exists(p))
            create_directory(p);
        
        FileStorage fs(filename, FileStorage::WRITE);

        fs << "FrameNumber" << (int)frameNumber; 
        fs << "Threshold"   << threshold  ; 
        fs << "Alpha"       << alpha      ; 
        fs << "Km"          << km         ; 
        fs << "Kv"          << kv         ; 
        fs << "Gaussians"   << (int)gaussians  ; 

        fs.release();

    }

}


string T2FMRF_UMBuilder::ElapsedTimeAsString()
{
    std::stringstream _elapsed ;
    _elapsed << duration ;
    return _elapsed.str() ;
}



