echo "=================== step1.1 Download taobao ad ctr dataset ==================="


echo "=================== step1.2. uncompress datasets ==================="
if [[ ! -f "./uncompress/behavior_log.csv" ]]; then
    cd uncompress/
    # tar xvf ../orig/behavior_log.csv.tar.gz 
    # tar xvf ../orig/ad_feature.csv.tar.gz 
    # tar xvf ../orig/user_profile.csv.tar.gz
    # tar xvf ../orig/raw_sample.csv.tar.gz 
    cd -
fi

echo "=================== step1.3 gen global user behavior feature from behavior_log.csv ==================="
# Note: This step needs main memorys more than 64GB, you can skip it.
python3 user_behavior_global.py

echo "=================== step1.4. gen local user behavior feature from raw_sample.csv ==================="
python3 user_behavior_local.py

echo "=================== step1.5 join ad features & user profile & global user feature & local user feature with raw_sample ==================="
if [[ ! -f "./final_data/train_data" ]]; then
    python3 gen_sample.py
    if [[ $? -eq 0 ]]; then
        echo "gen sample successfully"
    else
        echo "gen sample failed"
        exit
    fi
fi

echo "=================== step1.6 shuffle train/test data ==================="
if [[ ! -f "./final_data/train_data_shuf" ]]; then
    shuf final_data/train_data -o final_data/train_data_shuf
    shuf final_data/test_data -o final_data/test_data_shuf
fi