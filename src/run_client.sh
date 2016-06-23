#!/bin/bash

# Require a path where image sequences are located
MAIN_PATH="$HOME"

# Path to videos
SEQ_PATH="${MAIN_PATH}/Activity-AVIfiles"
#SEQ_PATH="${MAIN_PATH}/Activity-JPGfiles"
GT_PATH="${MAIN_PATH}/Ground-Truth"

# Place to save mask results 
MASK_PATH="${MAIN_PATH}/results/masks"
MEASURES="${MAIN_PATH}/results/measures"

# Algorithm name: 'sagmm' 'mog2' 'ucv'
#ALGORITHMS="sagmm mog2 ucv_linear ucv_staircase"
ALGORITHMS="imbs"

# Activities
ACTIONS="Kick Punch RunStop ShotGunCollapse WalkTurnBack"
ACTORS="Person1 Person4"
CAMERAS="Camera_3 Camera_4"
# end video definition

# Select run either algorithm or performance
run_algorithm="True"
run_performance="True"

# Binary command for performance
pmbgs="pmbgs"
pm_args=""

# Special tag
#TAG="AVI_MORPH"
#TAG="AVI"
TAG=""

###### From this point nothing should change .. 

# Ground-truth frames 
GT_Kick_Person1_Camera_3="2370 2911"
GT_Kick_Person1_Camera_4="2370 2911"
GT_Kick_Person4_Camera_3="200 628"
GT_Kick_Person4_Camera_4="200 628"
GT_Punch_Person1_Camera_3="2140 2607"
GT_Punch_Person1_Camera_4="2140 2607"
GT_Punch_Person4_Camera_3="92 536"
GT_Punch_Person4_Camera_4="92 536"
GT_RunStop_Person1_Camera_3="980 1418"
GT_RunStop_Person1_Camera_4="980 1418"
GT_RunStop_Person4_Camera_3="293 618"
GT_RunStop_Person4_Camera_4="293 618"
GT_ShotGunCollapse_Person1_Camera_3="267 1104"
GT_ShotGunCollapse_Person1_Camera_4="267 1104"
GT_ShotGunCollapse_Person4_Camera_3="319 1208"
GT_ShotGunCollapse_Person4_Camera_4="319 1208"
GT_WalkTurnBack_Person1_Camera_3="216 682"
GT_WalkTurnBack_Person1_Camera_4="216 682"
GT_WalkTurnBack_Person4_Camera_3="207 672"
GT_WalkTurnBack_Person4_Camera_4="207 672"
#

# fixed parameter
_header_tag="opencv_storage"



# Option specific for GMM: Verify of 'Gen' is less than 'Range'.
verify_sagmm_gen_value() {
    if [ "$ALGORITHM_NAME" == "sagmm" ]; then

        gen_tag="Gen"

        G=9.0 
        result=`echo $1'<'$G | bc -l`
        if [ $result -gt 0 ]; then
            gen_val=${1/.*}
        else
            gen_val=${G}
        fi

        cat ${xmlfile} | grep -v "/${_header_tag}\|${gen_tag}"             > config.tmp
        echo -e "<${gen_tag}>${gen_val}</${gen_tag}>\n</${_header_tag}>" >> config.tmp
        mv config.tmp ${xmlfile}
    fi
}

# Create mask Directory
create_mask_directory() {

    # MASK_PATH  --> {.../results/masks}
    # MEASURES   --> {.../results/measures}
    # input_name --> {Kick_Person4_Camera_3}
    # mask_name  --> {sagmm_Tubruk_L_0.001-0.005_T_2-100}
    input_name=$1
    new_mask_dir="${MASK_PATH}/${input_name}/${mask_name}"
    new_measure_dir="${MEASURES}/${input_name}/${mask_name}"


    # Delete previous link
    if [ -L "${mask_dir}" ]; then
        unlink ${mask_dir}
    fi
    if [ -L "${measure_dir}" ]; then
        unlink ${measure_dir}
    fi

    if [ ! -d "${new_mask_dir}" ]; then
        mkdir -p ${new_mask_dir}
    fi
    if [ ! -d "${new_measure_dir}" ]; then
        mkdir -p ${new_measure_dir}
    fi


    # mask_dir    --> {sagmm_mask}
    # measure_dir --> {sagmm_measure}
    # sagmm_mask    --> results/masks/Kick_Person4_Camera_3/sagmm_Tubruk_AVI_L_0.001-0.001_T_2-100
    # sagmm_measure --> results/measure/Kick_Person4_Camera_3/sagmm_Tubruk_AVI_L_0.001-0.001_T_2-100
    ln -s ${new_mask_dir} ${mask_dir}
    ln -s ${new_measure_dir} ${measure_dir}
}


set_ground_truth_frames() {

    eval FRAMES_VAL='$GT_'${name}

    # Get <init,end> ground truth pairs from variables defined at beginning 
    init_gt=`echo ${FRAMES_VAL} | awk '{print $1}'`
    end_gt=`echo  ${FRAMES_VAL} | awk '{print $2}'`

    # Set xml config file with previous values.
    cat ${xmlfile} | grep -v "/${_header_tag}\|InitFGMaskFrame\|EndFGMaskFrame" > config.tmp
    echo -e  "<InitFGMaskFrame>${init_gt}</InitFGMaskFrame>"                   >> config.tmp
    echo -e  "<EndFGMaskFrame>${end_gt}</EndFGMaskFrame>\n</${_header_tag}>"   >> config.tmp

    mv config.tmp ${xmlfile}

    framework="config/Framework.xml"

    LIST="mog2 np sagmm ucv_linear ucv_staircase"
    for i in ${LIST}
    do
        NAME=`echo ${i} | tr '[:lower:]' '[:upper:]'`
        ENABLE="0"
        if [ "${ALGORITHM_NAME}" == "${i}" ]; then
            ENABLE="1"
        fi

        cat ${framework} | grep -v "/${_header_tag}\|${NAME}"       > Framework.tmp
        echo -e  "<${NAME}>${ENABLE}</${NAME}>\n</${_header_tag}>" >> Framework.tmp
        mv Framework.tmp ${framework}
    done

    cat ${framework} | grep -v "/${_header_tag}\|InitFGMaskFrame\|EndFGMaskFrame" > Framework.tmp
    echo -e  "<InitFGMaskFrame>${init_gt}</InitFGMaskFrame>"                     >> Framework.tmp
    echo -e  "<EndFGMaskFrame>${end_gt}</EndFGMaskFrame>\n</${_header_tag}>"     >> Framework.tmp
    mv Framework.tmp ${framework}

}

# Set config names
set_names() {

    # input parameters
    input_name=`echo ${ALGORITHM_NAME} | tr '[:lower:]' '[:upper:]'`
    loop1="config/${input_name}_FgThreshold.txt"
    loop2="config/${input_name}_AssociationThreshold.txt"
    
    # get name of parameters (e.g: Alpha, Threshold)
    _tag_1=`head -1 $loop1 | sed -n 's|<\([a-zA-Z]*\)>\(.*\)</[a-zA-Z]*>|\1|p'`
    _tag_2=`head -1 $loop2 | sed -n 's|<\([a-zA-Z]*\)>\(.*\)</[a-zA-Z]*>|\1|p'`
    
    # build a list of values
    _list_1=`cat $loop1 | sed -n 's|<\([a-zA-Z]*\)>\(.*\)</[a-zA-Z]*>|\2|p' | tr '\n' ' '`
    _list_2=`cat $loop2 | sed -n 's|<\([a-zA-Z]*\)>\(.*\)</[a-zA-Z]*>|\2|p' | tr '\n' ' '`

    # Range of parameters
    range1="L_"$(head -1 ${loop1} | sed -n 's|<\([a-zA-Z]*\)>\(.*\)</[a-zA-Z]*>|\2|p')"-"$(tail -1 ${loop1} | sed -n 's|<\([a-zA-Z]*\)>\(.*\)</[a-zA-Z]*>|\2|p')
    range2="T_"$(head -1 ${loop2} | sed -n 's|<\([a-zA-Z]*\)>\(.*\)</[a-zA-Z]*>|\2|p')"-"$(tail -1 ${loop2} | sed -n 's|<\([a-zA-Z]*\)>\(.*\)</[a-zA-Z]*>|\2|p')

    #config file
    mask_dir="${ALGORITHM_NAME}_mask"
    mask_name="${ALGORITHM_NAME}_$(hostname)_${range1}_${range2}"
    measure_dir="${ALGORITHM_NAME}_measure"
    xmlfile="config/${ALGORITHM_NAME}.xml"

    if [ "${TAG}" != "" ]; then
        mask_name="${ALGORITHM_NAME}_$(hostname)_${TAG}_${range1}_${range2}"
    fi

    # Binary command: 'bgs', 'bgs_framework', 'testUCV'
    #Type of method: 1:linear 2:staircase 3:gmm normal
    ext_args="--show=false"
    if   [ "$ALGORITHM_NAME" == "sagmm" ]; then
        cmd="sagmm_bgs"
    elif [ "$ALGORITHM_NAME" == "mog2" ]; then
        cmd="bgs_framework"
    elif [ "$ALGORITHM_NAME" == "np" ]; then
        cmd="npbgs"
    elif [ "$ALGORITHM_NAME" == "ucv_linear" ]; then
        cmd="./bin/testUCV"
        ext_args="--function=1"
    elif [ "$ALGORITHM_NAME" == "ucv_staircase" ]; then
        cmd="./bin/testUCV"
        ext_args="--function=2"
    elif [ "$ALGORITHM_NAME" == "imbs" ]; then
        cmd="bgs_imbs"
    fi

}


verify_unexpected_algorithm_stop() {

    if [ -e "${hidden_name}" ]; then

        if [ -L "${mask_dir}" ]; then

            last_dir=`ls -1rt ${mask_dir} | tail -1`
            parameter_1=$(cat  ${mask_dir}/${last_dir}/parameters.txt | sed -n 's|.*\('${_tag_1}'\)=\([0-9]*\.\{0,1\}[0-9]*\).*|\2|p')
            parameter_2=$(cat  ${mask_dir}/${last_dir}/parameters.txt | sed -n 's|.*\('${_tag_2}'\)=\([0-9]*\.\{0,1\}[0-9]*\).*|\2|p')

            for p1 in ${_list_1}
            do
                [[ "${p1}" == "${parameter_1}" || "${new_list_1}" != "" ]] && new_list_1=$(echo $new_list_1 " " $p1)
            done

            for p2 in ${_list_2}
            do
                [[ "${p2}" == "${parameter_2}" || "${new_list_2}" != "" ]] && new_list_2=$(echo $new_list_2 " " $p2)
            done

            # Prepare new lists
            if [ "${new_list_1}" != "" ]; then
                tmp_list_1=${_list_1}
                _list_1=${new_list_1}
            fi
            if [ "${new_list_2}" != "" ]; then
                tmp_list_2=${_list_2}
                _list_2=${new_list_2}
            fi

            # Delete last directory
            rm -rf ${mask_dir}/${last_dir}
            rm output_*.txt

            echo "Restarting from previous execution"
            echo "List1: $parameter_1/$_list_1"
            echo "List2: $parameter_2/$_list_2"
            echo "TMP List1:$tmp_list_1"
            echo "TMP List2:$tmp_list_2"
        fi

        rm ${hidden_name}
        rm .running_*
    fi

}

check_aborted_execution() {

    hidden_file=$(ls -a1rt | grep '\.running_' | tail -1)
    # True if file exist
    if [ -n "$hidden_file" ]; then
        _last_algorithm=$(echo ${hidden_file} |  sed -n 's|.*running_\([a-z]*[2]\{0,1\}_\{0,1\}[a-z]\{0,\}\)_.*Person.*|\1|p')
        _last_action=$(   echo ${hidden_file} |  sed -n 's|.*running_\([a-z]*[2]\{0,1\}_\{0,1\}[a-z]\{0,\}\)_\([a-zA-Z]*\)_Person.*|\2|p')
        _last_actor=$(    echo ${hidden_file} |  sed -n 's|.*running.*_\(Person[1-4]\).*|\1|p')
        _last_camera=$(   echo ${hidden_file} |  sed -n 's|.*running.*_\(Camera_[3-4]\).*|\1|p')

        if [ "${_last_algorithm}" != "$(echo ${ALGORITHMS}|awk '{print $1}')" ]; then 
            for i in ${ALGORITHMS}
            do
                [[ "${i}" == "${_last_algorithm}" || "${new_algorithm_list}" != "" ]] && new_algorithm_list=$(echo $new_algorithm_list " " $i)
            done
        fi

        if [ "${_last_action}" != "$(echo ${ACTIONS}|awk '{print $1}')" ]; then 
            for i in ${ACTIONS}
            do
                [[ "${i}" == "${_last_action}" || "${new_action_list}" != "" ]] && new_action_list=$(echo $new_action_list " " $i)
            done
        fi


        [[ "${_last_actor}"  == "Person4" ]]  && new_actor_list=$(echo $_last_actor)
        [[ "${_last_camera}" == "Camera_4" ]] && new_camera_list=$(echo $_last_camera)

        if [ -n "${new_algorithm_list}" ]; then 
            tmp_algorithm_list=${ALGORITHMS}
            ALGORITHMS=${new_algorithm_list} 
        fi

        if [ -n "${new_action_list}"    ]; then
            tmp_action_list=${ACTIONS}
            ACTIONS=${new_action_list}
        fi

        if [ -n "${new_actor_list}"     ]; then 
            tmp_actor_list=${ACTORS}
            ACTORS=${new_actor_list}
        fi

        if [ -n "${new_camera_list}"    ]; then 
            tmp_camera_list=${CAMERAS}
            CAMERAS=${new_camera_list}
        fi

    fi

}

# Start script

# Verify previous execution was aborted
# to override this just delete '.running_*' file
check_aborted_execution

# Main Loop

for algorithm in ${ALGORITHMS}
do
    ALGORITHM_NAME=${algorithm}
    set_names

    echo "SUMMARY OF MAIN EXECUTION (`date`)"
    echo "---------------------------------------"
    echo "ALGORITHMS:${algorithm}/${ALGORITHMS}"
    echo "ACTIONS:${ACTIONS}"
    echo "ACTORS:${ACTORS}"
    echo "CAMERAS:${CAMERAS}"

    for action in ${ACTIONS}
    do
        for actor in ${ACTORS}
        do
            for cam in ${CAMERAS}
            do
                name="${action}_${actor}_${cam}" 
                set_ground_truth_frames
    
                # Link directories
                create_mask_directory $name
    
                # Sequence directory
                sequence="${SEQ_PATH}/${action}/${action}-${cam}-${actor}.avi"
                #sequence="${SEQ_PATH}/${action}/${actor}/${cam}"

                args="-i $sequence ${ext_args}"
                if [ "$ALGORITHM_NAME" == "imbs" ]; then
                    range_args=$(eval "echo \$$(echo GT_${name})| sed 's/ /,/g'")
                    args="-f ${sequence} ${ext_args} -r ${range_args}"
                fi

                hidden_name='.running_'${algorithm}'_'${name}

                verify_unexpected_algorithm_stop

                [ ! -e "${hidden_name}" ] && touch ${hidden_name}

                echo "SUMMARY OF MAIN PARAMETERS"
                echo "--------------------------"
                echo "LIST1:${_list_1}"
                echo "LIST2:${_list_2}"

                if [ "$run_algorithm" == "True" ]; then

                    # Process sequence actor camera
                    for in1 in ${_list_1}
                    do
                        cat ${xmlfile} | grep -v "/${_header_tag}\|${_tag_1}"       > config.tmp
                        echo -e "<${_tag_1}>${in1}</${_tag_1}>\n</${_header_tag}>" >> config.tmp
                        mv ${xmlfile} config.bak
                        mv config.tmp ${xmlfile}

                        for in2 in ${_list_2}
                        do
                            echo "Processing ${name} ${_tag_1}:${in1} ${_tag_2}:${in2}"

                            # Verify value of 'Gen' if less than 'Range'
                            verify_sagmm_gen_value $in2

                            cat ${xmlfile} | grep -v "/${_header_tag}\|${_tag_2}"       > config.tmp
                            echo -e "<${_tag_2}>${in2}</${_tag_2}>\n</${_header_tag}>" >> config.tmp
                            mv config.tmp ${xmlfile}

                            $cmd $args
                            #sleep 0.1
                        done

                        # Verify this started from previous execution for recovering _list_2.
                        if [ -n "${tmp_list_2}" ]; then
                            _list_2=${tmp_list_2}
                            tmp_list_2=""
                        fi
                    done

                    if [ -n "${tmp_list_1}" ]; then
                        _list_1=${tmp_list_1}
                        tmp_list_1=
                    fi
    
                fi

                if [ "$run_performance" == "True" ]; then

                    # Performance measure
                    # MASK_PATH  --> {.../results/masks}
                    # name       --> {Kick_Person1_Camera_3}
                    # mask_name  --> {sagmm_Tubruk_L_0.001-0.005_T_2-100}
                    if [ -d "${MASK_PATH}/${name}" ]; then

                        # set ground truth directory
                        camera=`echo ${cam} | sed s/_//`
                        ground_truth="${GT_PATH}/${action}${actor}${camera}"

                        mask="${MASK_PATH}/${name}/${mask_name}"
                        list_mask=$(ls -1rt ${mask} | tr '\n' ' ')

                        _list_perf=$(ls -1art | grep '\.performance_' | tail -1)
                        if [ -e "${_list_perf}" ]; then
                            _last_dir=$(echo ${_list_perf} | sed -n 's|.performance_.*_\([0-9]*\)|\1|p')

                            for i in ${list_mask}
                            do
                                [[ "${i}" == "${_last_dir}" || "${new_mask_list}" != "" ]] && new_mask_list=$(echo $new_mask_list " " $i)
                            done

                            list_mask=${new_mask_list}
                            new_mask_list=
                            rm output_*.txt

                        fi

                        for i in ${list_mask}
                        do
                            hidden_performance_name='.performance_'${algorithm}'_'${name}'_'${i}
                            touch ${hidden_performance_name}

                            input="${mask}/${i}"
                            args="-i ${input} -g ${ground_truth} ${_args}"
                            $pmbgs $args
                            mv -f output_*.txt ${measure_dir}
                            rm ${hidden_performance_name}
                        done

                    fi

                fi

                [ -e "${hidden_name}" ] && rm ${hidden_name}

                fs=`df -hl | grep /$ | tail -1 | awk '{print $5}' | sed 's/\%//'`
                while [ "$fs" -ge "80" ]
                do
                    current=$PWD
                    cd $MASK_PATH
                    del_dir=`ls -1r | tail -1`
                    rm -rf ${del_dir}
                    cd ${current}
                    fs=`df -hl | grep /$ | tail -1 | awk '{print $5}' | sed 's/\%//'`
                done

   
            done

            # Recovering cameras
            if [ -n "${tmp_camera_list}"    ]; then 
                echo "Recovering Camera List: ${tmp_camera_list}"
                CAMERAS=${tmp_camera_list}
                tmp_camera_list=
            fi
 
        done

        # Recovering actors
        if [ -n "${tmp_actor_list}"     ]; then 
            echo "Recovering Actor List: ${tmp_actor_list}"
            ACTORS=${tmp_actor_list}
            tmp_actor_list=
        fi

    done

    # Recovering actions
    if [ -n "${tmp_action_list}"    ]; then
        echo "Recovering Action List: ${tmp_action_list}"
        ACTIONS=${tmp_action_list}
        tmp_action_list=
    fi

done



