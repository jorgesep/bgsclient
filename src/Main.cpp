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

#include "BGSSystem.h"
#include "IMBSBuilder.h"
#include "FrameReaderFactory.h"
#include "DisplayImageUtils.h"

#include "utils.h"

using namespace cv;
using namespace std;
using namespace boost::filesystem;
using namespace seq;
using namespace bgs;

const std::string ALGORITHM_NAME = "IMBS";

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



int main( int argc, char** argv )
{
    // Setup algorithm name
    string algNameLowercase(ALGORITHM_NAME);
    transform(ALGORITHM_NAME.begin(), ALGORITHM_NAME.end(), 
              algNameLowercase.begin(),::tolower);

    //Parse console parameters
    CommandLineParser cmd(argc, argv, keys);

    // Reading input parameters
    const string inputName                = cmd.get<string>("input");
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
        input_frame = FrameReaderFactory::create_frame_reader(inputName);
    } catch (...) {
        cout << "Invalid file name "<< endl;
        return 0;
    }
   
    
    // Algorithm Instantiate
    BGSSystem* bgs = new BGSSystem();
    bgs->setAlgorithm(new IMBSBuilder());
    bgs->setName(ALGORITHM_NAME);
    bgs->loadConfigParameters();
    bgs->initializeAlgorithm();
    
    int  InitFGMaskFrame=0;
    int  EndFGMaskFrame = input_frame->getNFrames();

    // Prapare mask directory
    string _foreground_path    = algNameLowercase + "_mask";
    
    // Create foreground directory and numbered sub-directories (alg_mask/0, alg_mask/1, ...)
    if (saveForegroundMask) {

        create_foreground_directory(_foreground_path);

        std::ofstream outfile;
        stringstream param;

        param << _foreground_path << "/parameters.txt" ;
        outfile.open(param.str().c_str());
        outfile << bgs->getConfigurationParameters();
        outfile.close();

        if (!rangeSaveForegroundMask.empty()) {
            Point pf;
            pf = stringToPoint(rangeSaveForegroundMask);
            InitFGMaskFrame = pf.x;
            EndFGMaskFrame  = pf.y;
        }
    }

    Point pin(0,0);
    std::ofstream point_file;

    // Define a point to be pinned on a display window
    if ( !pinPoint.empty() ) {

        pin = stringToPoint(pinPoint);
        pin.x = pin.x > input_frame->getNumberCols() ? 
                        input_frame->getNumberCols(): pin.x;
        pin.y = pin.y > input_frame->getNumberRows() ? 
                        input_frame->getNumberRows(): pin.y;

    }

    Mat CurrentFrame;
    Mat Foreground;
    Mat Image;
   
    int delay = input_frame->getFrameDelay();
    int cnt = 0;

    // main loop
    for(;;)
    {

        Foreground = Scalar::all(0);

        input_frame->getFrame(CurrentFrame);
        
        if (CurrentFrame.empty()) break;
    
        bgs->updateAlgorithm(CurrentFrame, Foreground);
        
        // Save foreground images
        if (saveForegroundMask && cnt >= InitFGMaskFrame && cnt <= EndFGMaskFrame) {
            stringstream str;
            vector<int> compression_params;
            compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
            compression_params.push_back(9);

            try {
                str << _foreground_path << "/" <<  cnt << ".png";
                imwrite(str.str(), Foreground, compression_params);
            }
            catch (runtime_error& ex) {
                cout << "Exception converting image to PNG format: " << ex.what() << endl;
            }
            catch (...) {
                cout << "Unknown Exception converting image to PNG format: " << endl;
            }
        }
        
        // Save pixel information in a local file
        if (!pinPoint.empty()) {

            stringstream msgPoint;
            msgPoint  << (int)CurrentFrame.at<Vec3b>(pin)[0] << " "
                      << (int)CurrentFrame.at<Vec3b>(pin)[1] << " "
                      << (int)CurrentFrame.at<Vec3b>(pin)[2] ;

            //define outfile to save BGR pixel values
            if (!point_file.is_open()){
                stringstream name("");
                name << "point_" << pin.x << "_" << pin.y << ".txt";
                point_file.open(name.str().c_str());
            }

            point_file<< msgPoint.str() << endl;
        }

        if (showWindow) {

            // Insert pin on the Window.
            if (!pinPoint.empty()){
                circle(CurrentFrame,pin,8,Scalar(0,0,254),-1,8);
                circle(Foreground  ,pin,8,Scalar(0,0,254),-1,8);
            }

            // Invert color of Mask from black to white
            Mat ThresholdMask, ColorMask;
            threshold(Foreground, ThresholdMask, 127, 255, 1);
            cvtColor(ThresholdMask,ColorMask, CV_GRAY2BGR);

            DisplayImages display;
            display.mergeImages(CurrentFrame, ColorMask, Image);

            imshow(ALGORITHM_NAME, Image);
            
            // Stop window
            char key=0;
            key = (char)waitKey(delay);
            if( key == 27 )
                break;

            // pause program in with space key
            if ( key == 32) {
                bool pause = true;
                while (pause)
                {
                    key = (char)waitKey(delay);
                    if (key == 32) pause = false;

                    // save frame with return key
                    if (key == 13) {
                        stringstream str;
                        str << algNameLowercase << "_" << cnt << "_background.png" ;
                        imwrite( str.str()  , CurrentFrame  );

                        str.str("") ;
                        str << algNameLowercase << "_" << cnt << "_foreground.png" ;
                        imwrite( str.str(), ThresholdMask );
                    }
                }
            }
        }


        cnt +=1;

        if (!showWindow && cnt > EndFGMaskFrame) break;
        
    }
    
    
    delete bgs;
    delete input_frame;
    if (point_file.is_open()) point_file.close();

    return 0;
}

