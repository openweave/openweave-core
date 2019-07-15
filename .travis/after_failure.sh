#!/bin/sh

#
#    Copyright 2018-2019 Google LLC All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      This file is the script for Travis CI hosted, distributed continuous
#      integration 'after_failure' trigger of 'failure' step.
#	   How dpl works: https://github.com/travis-ci/dpl
#	   How to encrypt gcs key: https://docs.travis-ci.com/user/encryption-keys/#usage

sudo apt-get remove ruby
sudo apt update
sudo apt install git curl libssl-dev libreadline-dev zlib1g-dev autoconf bison build-essential libyaml-dev libreadline-dev libncurses5-dev libffi-dev libgdbm-dev

cd $HOME
git clone https://github.com/travis-ci/dpl.git
cd $HOME/dpl/bin
gem install dpl
gem install bundler
bundle update

dpl --provider=gcs --access-key-id=GOOGNPZYTVTEPZM5VBRAVVUU \
	--secret-access-key=$GCSKEY --bucket=openweave \
	--local-dir=$TRAVIS_BUILD_DIR/happy-test-logs --upload-dir=happy-test-log/$TRAVIS_BUILD_NUMBER \
	--acl=public-read --skip_cleanup=true
