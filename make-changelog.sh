#!/bin/bash
git --no-pager log --since "1970" --format="%ai %aN (%h)%n%n%x09*%w(68,0,10) %s%d%n" > ChangeLog
