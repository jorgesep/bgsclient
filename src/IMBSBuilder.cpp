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
#include "IMBSBuilder.h"


const string       IMBSBuilder::AlgorithmName                 = "IMBS";
const double       IMBSBuilder::DefaultFps                    = 10.;
const unsigned int IMBSBuilder::DefaultFgThreshold            = 15;
const unsigned int IMBSBuilder::DefaultAssociationThreshold   = 5;
const double       IMBSBuilder::DefaultSamplingPeriod         = 250.;//500.ms
const unsigned int IMBSBuilder::DefaultMinBinHeight           = 2;
const unsigned int IMBSBuilder::DefaultNumSamples             = 10;
const double       IMBSBuilder::DefaultAlpha                  = 0.65f;
const double       IMBSBuilder::DefaultBeta                   = 1.15f;
const double       IMBSBuilder::DefaultTau_s                  = 60.;
const double       IMBSBuilder::DefaultTau_h                  = 40.;
const double       IMBSBuilder::DefaultMinArea                = 30.;
const double       IMBSBuilder::DefaultPersistencePeriod      = DefaultSamplingPeriod*DefaultNumSamples/3.;
const bool         IMBSBuilder::DefaultMorphologicalFiltering = false;


IMBSBuilder::IMBSBuilder()
{
    loadDefaultParameters();
    cols = 0;
    rows = 0;
    nchannels = 0;
    has_been_initialized = false;
    frame_counter = 0;

}

IMBSBuilder::IMBSBuilder(int _col, int _row, int _nchannels)
{
    loadDefaultParameters();
    cols = _col;
    rows = _row;
    nchannels = _nchannels;
    has_been_initialized = false;
    frame_counter = 0;

}

IMBSBuilder::IMBSBuilder(Size _size, int _type)
{
    loadDefaultParameters();
    frameSize = _size;
    frameType = _type;
    rows = frameSize.height;
    cols = frameSize.width;
    nchannels = CV_MAT_CN(frameType);
    has_been_initialized = false;
    frame_counter = 0;
}


IMBSBuilder::~IMBSBuilder()
{
    if (model != NULL) {
        delete model;
    }
    model = 0;
}

void IMBSBuilder::Initialization()
{

    if (rows != 0 && cols != 0 && nchannels != 0) {
        model = new BackgroundSubtractorIMBS(fps,
                                             fgThreshold,
                                             associationThreshold,
                                             samplingPeriod,
                                             minBinHeight,
                                             numSamples,
                                             alpha,
                                             beta,
                                             tau_s,
                                             tau_h,
                                             minArea,
                                             persistencePeriod,
                                             morphologicalFiltering);

        model->initialize(frameSize, frameType);
        has_been_initialized = true;
    }
    
}

void IMBSBuilder::GetBackground(OutputArray image) 
{

    Mat img_background;
    model->getBackgroundImage(img_background);
    img_background.copyTo(image);

}

void IMBSBuilder::GetForeground(OutputArray mask) 
{

}

void IMBSBuilder::Update(InputArray frame, OutputArray mask) 
{
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

}


void IMBSBuilder::loadDefaultParameters()
{

    fps                    = DefaultFps; 
    fgThreshold            = DefaultFgThreshold;
    associationThreshold   = DefaultAssociationThreshold;
    samplingPeriod         = DefaultSamplingPeriod; 
    minBinHeight           = DefaultMinBinHeight;
    numSamples             = DefaultNumSamples;
    alpha                  = DefaultAlpha;
    beta                   = DefaultBeta;
    tau_s                  = DefaultTau_s;
    tau_h                  = DefaultTau_h;
    minArea                = DefaultMinArea;
    persistencePeriod      = DefaultPersistencePeriod;
    morphologicalFiltering = DefaultMorphologicalFiltering;
}

string IMBSBuilder::PrintParameters()
{
    std::stringstream str;
    str
    << "# " 
    << "Fps="                    << fps                      << " " 
    << "FgThreshold="            << fgThreshold              << " " 
    << "AssociationThreshold="   << associationThreshold     << " " 
    << "SamplingPeriod="         << samplingPeriod           << " " 
    << "MinBinHeight="           << minBinHeight             << " " 
    << "NumSamples="             << numSamples               << " " 
    << "Alpha="                  << alpha                    << " " 
    << "Beta="                   << beta                     << " " 
    << "Tau_s="                  << tau_s                    << " " 
    << "Tau_h="                  << tau_h                    << " " 
    << "MinArea="                << minArea                  << " " 
    << "PersistencePeriod="      << persistencePeriod        << " " 
    << "MorphologicalFiltering=" << morphologicalFiltering; 

    return str.str();

}

void IMBSBuilder::LoadConfigParameters()
{
    string nameLowercase(AlgorithmName);
    std::transform(AlgorithmName.begin(),
                   AlgorithmName.end(),
                   nameLowercase.begin(),
                   ::tolower);

    string filename = "config/" + nameLowercase + ".xml";

    if ( exists(filename) && is_regular_file(filename) ) {
        
        FileStorage fs(filename, FileStorage::READ);
       
        fps                           = (int)fs["Fps"];
        fgThreshold                   = (int)   fs["FgThreshold"];
        associationThreshold          = (int)   fs["AssociationThreshold"];
        samplingPeriod                = (double)fs["SamplingPeriod"];
        minBinHeight                  = (int)   fs["MinBinHeight"];
        numSamples                    = (int)   fs["NumSamples"];
        alpha                         = (double)fs["Alpha"];
        beta                          = (double)fs["Beta"];
        tau_s                         = (double)fs["Tau_s"];
        tau_h                         = (double)fs["Tau_h"];
        minArea                       = (double)fs["MinArea"];
        persistencePeriod             = (double)fs["PersistencePeriod"];
        morphologicalFiltering        = (int)   fs["MorphologicalFiltering"];

        cout << "Just for debugging: " << persistencePeriod << endl;
        cout << "Just for debugging: " << fps << endl;

        fs.release();
    }
    else {
        path p("config");
        if (!exists(p))
            create_directory(p);
        
        FileStorage fs(filename, FileStorage::WRITE);

        fs << "Fps"                    << fps                      ; 
        fs << "FgThreshold"            << (int)fgThreshold         ; 
        fs << "AssociationThreshold"   << (int)associationThreshold; 
        fs << "SamplingPeriod"         << samplingPeriod           ; 
        fs << "MinBinHeight"           << (int)minBinHeight        ; 
        fs << "NumSamples"             << (int)numSamples          ; 
        fs << "Alpha"                  << alpha                    ; 
        fs << "Beta"                   << beta                     ; 
        fs << "Tau_s"                  << tau_s                    ; 
        fs << "Tau_h"                  << tau_h                    ; 
        fs << "MinArea"                << minArea                  ; 
        fs << "PersistencePeriod"      << persistencePeriod        ; 
        fs << "MorphologicalFiltering" << (int)morphologicalFiltering; 

        fs.release();

    }

}


string IMBSBuilder::ElapsedTimeAsString()
{
    std::stringstream _elapsed ;
    _elapsed << duration ;
    return _elapsed.str() ;
}



