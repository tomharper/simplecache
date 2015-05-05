#brew install python
easy_install virtualenv
virtualenv memc
source memc/bin/activate
pip install -r requirements.txt
deactivate
