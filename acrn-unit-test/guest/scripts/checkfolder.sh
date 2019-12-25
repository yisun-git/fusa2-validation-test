#!/bin/bash
path=$1

check(){
    filePath=$1
    echo ""
    echo "*********************************************************************" 
    echo "start to check $filePath" 
    ./checkpatch.pl -f $filePath
    if [[ "$?" == "1"  ]];then
    	result=1
    fi
}

read_dir(){
    for file in `ls $1`
    do
    	if [ -d $1"/"$file ];then
                #echo "++++++++++++++: "$1"/"$file
        	read_dir $1"/"$file
    	elif [[ $1"/"$file =~ ".c" ]] || [[ $1"/"$file =~ ".h" ]];then
    		#echo "----------------: "$1"/"$file 
		check $1"/"$file
    	fi
    done
}

return_message(){
    echo "**********result: $result"
    if [[ "$result" == "1"  ]];then
    	return $result
    else
     	return $?	
    fi
}

main(){
if [[ $path == "" ]] || [[ $path == null ]];then
    echo "which file or dir  need check , please write it as parameter" && exit 1
elif [[ $path =~ ".c" ]] || [[ $path =~ ".h" ]];then
    echo "will check $path"
    check $path
else
    echo "will check all file(.c and .h) in $path "
    read_dir $path
fi
return_message
}

main
