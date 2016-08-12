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

#include <stdio.h>
#include <opencv2/opencv.hpp>

#include <boost/filesystem.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <thread>

#include "BGSSystem.h"
#include "T2FGMM_UMBuilder.h"
#include "FrameReaderFactory.h"
#include "DisplayImageUtils.h"

#include "utils.h"

using namespace cv;
using namespace std;
using namespace boost::filesystem;
using namespace seq;
using namespace bgs;

const std::string ALGORITHM_NAME = "T2FGMM_UM";
const int NUM_THREADS = 16;
std::mutex mtx;

struct MainParams {
    MainParams(string an, string in, string pp, string mr, 
               bool eg, bool sm, int r, int c, int f, int d)
        :algName(an), fileName(in),pinPoint(pp), maskRange(mr),pin(0,0), 
         startFrameMask(0),endFrameMask(f),
         enableGui(eg), enableMask(sm), row(r), col(c), nframes(f), delay(d)
    {
        // Define a pin point location on a display window
        if ( !pinPoint.empty() ) {
            pin = stringToPoint(pinPoint);
            pin.x = pin.x > col ?  col: pin.x;
            pin.y = pin.y > row ?  row: pin.y;
        }

        if (!maskRange.empty()) {
            Point pf;
            pf = stringToPoint(maskRange);
            startFrameMask = pf.x;
            endFrameMask   = pf.y;
        }

        maskPath    = algName + "_mask";
    };
    
   const string algName; // algorithm name
   string configParams;
   string fileName; // video filename
   string pinPoint; //disply spot in video sequence
   string maskRange; // init and end mask frame to save
   string maskPath;
   Point  pin;
   int    startFrameMask;
   int    endFrameMask;
   bool   enableGui; // display video
   bool   enableMask; // save generated masks
   int row;
   int col;
   int nframes;
   int delay;
} ;


const char* keys =
{
    "{ f | input     |       | Input video }"
    "{ m | mask      | true  | Save foreground masks}"
    "{ s | show      | true  | Show display window}"
    "{ p | point     |       | Pin a red dot on the images for debugging,  e.g -p 250,300 }"
    "{ r | range     |       | Select a valid range save foreground masks}"
    "{ h | help      | false | Print help message }"
};

void display_message()
{
    cout << "Background Subtraction Evaluation System.                " << endl;
    cout << "------------------------------------------------------------" << endl;
    cout << "Process input video comparing with its ground truth.        " << endl;
    cout << "OpenCV Version : "  << CV_VERSION << endl;
    cout << "Example:                                                    " << endl;
    cout << "bgs_client -i movie_file                               " << endl << endl;
    cout << "------------------------------------------------------------" << endl <<endl;
}

void save_foreground_mask(const MainParams p, int cnt, InputArray im)
{
    // Save foreground images
    if (p.enableMask && cnt >= p.startFrameMask && cnt <= p.endFrameMask) {

        Mat Mask = im.getMat();

        stringstream str;
        vector<int> compression_params;
        compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
        compression_params.push_back(9);

        try {
            str << p.maskPath << "/" <<  cnt << ".png";
            imwrite(str.str(), Mask, compression_params);
        }
        catch (runtime_error& ex) {
            cout << "Exception converting image to PNG format: " << ex.what() << endl;
        }
        catch (...) {
            cout << "Unknown Exception converting image to PNG format: " << endl;
        }
    }
}
 
void save_pixel_values_tofile(const MainParams p, InputArray im, std::ofstream& file)
{
    // Save pixel information in a local file
    if (!p.pinPoint.empty()) {

        Mat Img = im.getMat();

        stringstream msgPoint;
        msgPoint  << (int)Img.at<Vec3b>(p.pin)[0] << " "
                  << (int)Img.at<Vec3b>(p.pin)[1] << " "
                  << (int)Img.at<Vec3b>(p.pin)[2] ;

        //define outfile to save BGR pixel values
        if (!file.is_open()){
            stringstream name("");
            name << "point_" << p.pin.x << "_" << p.pin.y << ".txt";
            file.open(name.str().c_str());
        }
        file<< msgPoint.str() << endl;
    }
}
 

void setup_foreground_directory(MainParams &p)
{
    // Create foreground directory and numbered sub-directories (alg_mask/0, alg_mask/1, ...)
    if (p.enableMask) 
    {

        // Setup algorithm name
        string algNameLowercase(ALGORITHM_NAME);
        transform(ALGORITHM_NAME.begin(), 
                  ALGORITHM_NAME.end(), 
                  algNameLowercase.begin(),::tolower);

        // Prapare mask directory
        string _foreground_path    = algNameLowercase + "_mask";

        create_foreground_directory(_foreground_path);

        std::ofstream outfile;
        stringstream param;

        param << _foreground_path << "/parameters.txt" ;
        outfile.open(param.str().c_str());
        outfile << p.configParams;
        outfile.close();
        p.maskPath = _foreground_path;
    }
}

bool display_images(MainParams &p, int cnt, InputArray im, InputArray fg)
{

    if (p.enableGui) {

        Mat Img  = im.getMat();
        Mat Mask = fg.getMat();
        Mat Image;
    
        // Insert pin on the Window.
        if (!p.pinPoint.empty()){
            circle(Img,  p.pin,8,Scalar(0,0,254),-1,8);
            circle(Mask, p.pin,8,Scalar(0,0,254),-1,8);
        }
    
        // Invert color of Mask from black to white
        Mat ThresholdMask, ColorMask;
        threshold(Mask, ThresholdMask, 127, 255, 1);
        cvtColor(ThresholdMask,ColorMask, CV_GRAY2BGR);
    
        DisplayImages display;
        display.mergeImages(Img, ColorMask, Image);
    
        imshow(ALGORITHM_NAME, Image);
        
        // Stop window
        char key=0;
        key = (char)waitKey(p.delay);
        if( key == 27 )
            return false;
    
        // pause program in with space key
        if ( key == 32) {
            bool pause = true;
            while (pause)
            {
                key = (char)waitKey(p.delay);
                if (key == 32) pause = false;
    
                // save frame with return key
                if (key == 13) {
                    stringstream str;
                    str << p.algName << "_" << cnt << "_background.png" ;
                    imwrite( str.str()  , Img  );
    
                    str.str("") ;
                    str << p.algName << "_" << cnt << "_foreground.png" ;
                    imwrite( str.str(), ThresholdMask );
                }
            }
        }
    }

    return true;


}


void process_images(BGSSystem* method, InputArray Img, OutputArray Mask)
{
    Mat CurrentFrame = Img.getMat();

    //mtx.lock();
    //std::cout << "function: process_images img[" << CurrentFrame.cols << ":" << CurrentFrame.rows << "] " << method->getConfigurationParameters()  << std::endl;
    //mtx.unlock();

    method->updateAlgorithm(CurrentFrame, Mask);


}


int main( int argc, char** argv )
{
    // Setup algorithm name
    string algNameLowercase(ALGORITHM_NAME);
    transform(ALGORITHM_NAME.begin(), 
              ALGORITHM_NAME.end(), 
              algNameLowercase.begin(),::tolower);

    //Parse console parameters
    CommandLineParser cmd(argc, argv, keys);

    // Reading input parameters
    const string fileName                = cmd.get<string>("input");
    const bool showWindow                 = cmd.get<bool>("show");
    const string pinPoint                 = cmd.get<string>("point");
    const bool saveForegroundMask         = cmd.get<bool>("mask");
    const string  rangeSaveForegroundMask = cmd.get<string>("range");
    
    // Show help not input options
    if (cmd.get<bool>("help")) {
        display_message();
        cmd.printParams();
        return 0;
    }
    
    // Verify input name is a video file or directory with image files.
    FrameReader *input_frame;
    try {
        input_frame = FrameReaderFactory::create_frame_reader(fileName);
    } catch (...) {
        cout << "Invalid file name "<< endl;
        return 0;
    }
 
    MainParams params(
            algNameLowercase, 
            fileName, 
            pinPoint, 
            rangeSaveForegroundMask,
            showWindow, 
            saveForegroundMask, 
            input_frame->getNumberRows(), 
            input_frame->getNumberCols(), 
            input_frame->getNFrames(),
            input_frame->getFrameDelay());


    // Create vector with number of theads
    ChunkImage chunk(input_frame->getNumberRows(), input_frame->getNumberCols(), NUM_THREADS);

    //std::cout << "Chunk size  [" << chunk.getSubImgCol() << ":" << chunk.getSubImgRow() << "]" << std::endl;



    vector<BGSSystem*> methods;
    for (int i=0; i<NUM_THREADS; i++) {
        // Algorithm Instantiate
        BGSSystem* bgs = new BGSSystem();
        bgs->setAlgorithm(new T2FGMM_UMBuilder(chunk.getSubImgCol(),
                                               chunk.getSubImgRow(),
                                               input_frame->getNChannels()   ));
        bgs->setName(ALGORITHM_NAME);
        bgs->loadConfigParameters();
        bgs->initializeAlgorithm();
 
        methods.push_back(bgs);
        if (i == 0 )
            params.configParams = bgs->getConfigurationParameters();
    }

    // Create foreground directory and numbered sub-directories (alg_mask/0, alg_mask/1, ...)
    setup_foreground_directory(params);

    std::ofstream point_file;
    Mat CurrentFrame;
    Mat Foreground;
    Mat Image;
   
    int cnt = 0;

    std::vector<Mat> subImgs;
    std::vector<Mat> subMask;
    std::vector<std::thread> t;

    //Initialize vector of masks
    for (int i=0; i<NUM_THREADS; i++ ){
        Size size(chunk.getSubImgCol(),chunk.getSubImgRow());
        subMask.push_back(Mat(size,CV_8UC1,Scalar::all(0)));
    }

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    // main loop
    for(;;)
    {

        Foreground = Scalar::all(0);
        input_frame->getFrame(CurrentFrame);
        if (CurrentFrame.empty()) break;

        chunk(CurrentFrame,subImgs);

        //Launch a group of threads
        for (int i = 0; i < NUM_THREADS; ++i) 
            t.push_back(std::thread(process_images,methods[i], subImgs[i], subMask[i]));

        //Join the threads with the main thread
        for(auto &e : t){
            e.join();
        }

        t.clear();


        chunk.mergeImages(subMask,Foreground);
        save_foreground_mask(params,cnt, Foreground);

        save_pixel_values_tofile(params, CurrentFrame, point_file);

        if (!display_images(params, cnt, CurrentFrame, Foreground)) break;

        cnt +=1;

        subImgs.clear();

        if (!showWindow && cnt > params.endFrameMask) break;
        
    }
    
    std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << "[Sec]" << std::endl;
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<std::endl;
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<std::endl;
    
    for (int i=0; i<NUM_THREADS; i++) {
        delete methods[i];
    }
    delete input_frame;
    if (point_file.is_open()) point_file.close();

    return 0;
}

