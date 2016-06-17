#!/bin/bash

# Require a path where image sequences are located
MAIN_PATH="Path to some specific directory"

# Path to videos
GT_PATH="${MAIN_PATH}/Ground-Truth"
MASK_PATH="${PWD}/results/masks"
MEASURES="${PWD}/results/measures"

# Actions definition
ACTIONS="Kick Punch RunStop ShotGunCollapse WalkTurnBack"
ACTORS="Person1 Person4"
CAMERAS="Camera_3 Camera_4"
# end video definition

# Algorithm name: 'sagmm' 'mog2' 'ucv'
ALGORITHM_NAME="imbs"

# Binary command
cmd="pmbgs"
ext_args=""

###### From this point nothing should change ..


#config file
mask_dir="${ALGORITHM_NAME}_mask"
measure_dir="${ALGORITHM_NAME}_measure"


# Create mask Directory
create_mask_directory() {

    name=$1
    seq_mask_dir="${MASK_PATH}/${name}"
    mask="${seq_mask_dir}/${mask_dir}"
    current="$PWD"

    # Delete previous link
    if [ -L "${measure_dir}" ]; then
        unlink ${measure_dir}
    fi

    if [ -L "${mask}" ]; then
        seq_name=`ls -lah ${MASK_PATH}/${name} | grep ^l | awk '{print $11}'`
    else
        seq_name=`ls -lrt ${MASK_PATH}/${name} |  awk '{print $9}' | tail -1`

        # Create mask_dir link to last created directory
        cd ${seq_mask_dir}
        ln -s ${seq_name} ${mask_dir}
        cd ${current}
    fi

    new_dir="${MEASURES}/${name}/${seq_name}"
    if [ ! -d "${new_dir}" ]; then
        mkdir -p ${new_dir}
    fi

    ln -s ${new_dir} ${measure_dir}

}



# Loop for each action
for action in ${ACTIONS}
do
    for actor in ${ACTORS}
    do
        for cam in ${CAMERAS}
        do
            name="${action}_${actor}_${cam}" 

            # temporary work around
            #if [ "$name" == "Kick_Person1_Camera_3" ]; then
            #    continue  ### resumes iteration of an enclosing for loop ###
            #fi

            # set ground truth directory
            camera=`echo ${cam} | sed s/_//`
            ground_truth="${GT_PATH}/${action}${actor}${camera}"

            # set output directory
            create_mask_directory $name

            mask="${MASK_PATH}/${name}/${mask_dir}"
            list_mask=`ls ${mask} | sort -n`

            echo $mask
            echo $ground_truth

            for i in ${list_mask}
            do
                input="${mask}/${i}"
                args="-i ${input} -g ${ground_truth} ${ext_args}"
                $cmd $args
                mv output_*.txt ${measure_dir}
            done

            unlink ${measure_dir}

        done
    done
done
