#!/bin/bash
path=$1
result=0

check(){
    filePath=$1
    echo ""
    echo "*********************************************************************" 
    echo "start to check $filePath" 
    ./checkpatch.pl -f $filePath
    if [[ "$?" != "0" ]];then
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
    return $result
}

main(){
log=`mktemp /tmp/checkpatch.XXXXX`
if [[ $path == "" ]] || [[ $path == null ]];then
    echo "which file or dir  need check , please write it as parameter" && exit 1
elif [[ $path =~ ".c" ]] || [[ $path =~ ".h" ]];then
    echo "will check $path"
    check $path &>$log
else
    echo "will check all file(.c and .h) in $path "
    read_dir $path &>$log
fi
sed -e '/^NOTE: /d' -e '/^No typos will be found - /d' -e '/^No structs that should be const will be found - /d' $log
rm -f $log
return_message
}

main
