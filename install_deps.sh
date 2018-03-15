#!/bin/bash

# List of useful colors
COLOR_RESET="\033[0m"
COLOR_INFO="\033[0;32m"
COLOR_ITEM="\033[1;34m"
COLOR_QUES="\033[1;32m"
COLOR_WARN="\033[0;33m"
COLOR_BOLD="\033[1m"
COLOR_UNDE="\033[4m"

if [[ $1 == "autoinstall" ]]; then
   AUTOINSTALL=true
else
   AUTOINSTALL=false
fi

# Getting the current directory of this script
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


## This function detects the current os and distro
CURRENT_OS="Unsupported" #CENTOS, UBUNTU are other valid options
function findCurrentOSType()
{
	echo
	osType=$(uname)
	case "$osType" in
	"Linux")
	{
		# If available, use LSB to identify distribution
		if [ -f /etc/lsb-release -o -d /etc/lsb-release.d ]; then
			DISTRO=$(gawk -F= '/^NAME/{print $2}' /etc/os-release) # try to remove the ""
		else
			DISTRO=$(ls -d /etc/[A-Za-z]*[_-][rv]e[lr]* | grep -v "lsb" | cut -d'/' -f3 | cut -d'-' -f1 | cut -d'_' -f1)
		fi
		CURRENT_OS=$(echo $DISTRO | tr 'a-z' 'A-Z')
		CURRENT_OS="UBUNTU"
	} ;;
	*)
	{
		echo "Unsupported OS, exiting"
		exit
	} ;;
	esac
	echo -e "${COLOR_BOLD}Running on ${CURRENT_OS}.${COLOR_RESET}"
}

function install_lapack
{
	# Install the packages
	sudo apt-get --yes -install liblapack-dev liblapack3 libblas-common libblas-dev libblas3
}

function install_rbdl
{
	if [ "$1" == "purge" ]; then
		echo -e "${COLOR_BOLD}Purging dls-rbdl...${COLOR_RESET}"
		# Purge the package first
		sudo apt-get --yes purge dls-rbdl
	fi
	echo -e "${COLOR_BOLD}Installing dls-rbdl...${COLOR_RESET}"
	# Then install the package
	sudo apt-get --yes install dls-rbdl
}

function install_swig
{
	if [ "$1" == "purge" ]; then
		echo -e "${COLOR_BOLD}Purging dls-swig...${COLOR_RESET}"
		# Purge the package first
		sudo apt-get --yes purge dls-swig
	fi
	echo -e "${COLOR_BOLD}Installing dls-swig...${COLOR_RESET}"
	# Then install the package
	sudo apt-get --yes install dls-swig
}

function install_ipopt
{
	if [ "$1" == "purge" ]; then
		echo -e "${COLOR_BOLD}Purging dls-libipopt...${COLOR_RESET}"
		# Purge the package first
		sudo apt-get --yes purge dls-libipopt
	fi
	echo -e "${COLOR_BOLD}Installing dls-libipopt...${COLOR_RESET}"
	# Then install the package
	sudo apt-get --yes install dls-libipopt
}

function install_qpoases
{
	if [ "$1" == "purge" ]; then
		echo -e "${COLOR_BOLD}Purging dls-qpoases...${COLOR_RESET}"
		# Purge the package first
		sudo apt-get --yes purge dls-qpoases
	fi
	echo -e "${COLOR_BOLD}Installing dls-qpoases...${COLOR_RESET}"
	# Then install the package
	sudo apt-get --yes install dls-qpoases
}

function install_libcmaes
{
	if [ "$1" == "purge" ]; then
		echo -e "${COLOR_BOLD}Purging dls-libcmaes...${COLOR_RESET}"
		# Purge the package first
		sudo apt-get --yes purge dls-libcmaes
	fi
	echo -e "${COLOR_BOLD}Installing dls-libcmaes...${COLOR_RESET}"
	# Then install the package
	sudo apt-get --yes install dls-libcmaes
}

function install_pyadolc
{
	if [ "$1" == "purge" ]; then
		echo -e "${COLOR_BOLD}Purging dls-pyadolc...${COLOR_RESET}"
		# Purge the package first
		sudo apt-get --yes purge dls-pyadolc
	fi
	echo -e "${COLOR_BOLD}Installing dls-pyadolc...${COLOR_RESET}"
	# Then install the package
	sudo apt-get --yes install dls-pyadolc
}

##############################################  MAIN  ########################################################
# Printing the information of the shell script
echo -e "${COLOR_BOLD}install_deps.sh - DWL Installation Script for Ubuntu Xenial Xerus 16.04${COLOR_RESET}"
echo ""
echo "Copyright (C) 2015-2018 Carlos Mastalli, <carlos.mastalli@laas.fr>"
echo "All rights reserved."
echo "Released under the BSD 3-Clause License."
echo ""
echo "This program comes with ABSOLUTELY NO WARRANTY."
echo "This is free software, and you are welcome to redistribute it"
echo "under certain conditions; see the LICENSE text file for more information."

# This file is part of DWL Installer.
#
# DWL Installer is free software: you can redistribute it and/or
# modify it under the terms of the BSD 3-Clause License


## Detecting the current OS and distro
findCurrentOSType

sudo apt-get update

# Added doxygen install.
sudo apt-get install doxygen libeigen3-dev libyaml-cpp0.5v5

# Install lapack and blas libraries
install_lapack

# Added doxygen install. TODO moved from here and tested for other OS (i.e. Mac OSX)
sudo apt-get install doxygen
sudo apt-get install python2.7-dev python-numpy
sudo pip install --user numpy
sudo apt-get -qq install graphviz

##---------------------------------------------------------------##
##----------------------- Installing RBDL -----------------------##
##---------------------------------------------------------------##
echo ""
echo -e "${COLOR_BOLD}Installing RBDL (REQUIRED Package)...${COLOR_RESET}"
if $AUTOINSTALL; then
	install_rbdl purge
else
	PKG_OK=$(dpkg-query -W --showformat='${Status}\n' dls-rbdl|grep "install ok installed")
	if [ "" != "$PKG_OK" ]; then
		echo -e -n "${COLOR_QUES}Do you want to re-install RBDL? [y/N]: ${COLOR_RESET}"
		read ANSWER_RBDL
		if [ "$ANSWER_RBDL" == "Y" ] || [ "$ANSWER_RBDL" == "y" ]; then
			install_rbdl purge
		fi
	else
		echo -e -n "${COLOR_QUES}Do you want to install RBDL? [y/N]: ${COLOR_RESET}"
		read ANSWER_RBDL
		if [ "$ANSWER_RBDL" == "Y" ] || [ "$ANSWER_RBDL" == "y" ]; then
			install_rbdl
		fi
	fi
fi

##---------------------------------------------------------------##
##------------------------ Installing SWIG ----------------------##
##---------------------------------------------------------------##
echo ""
echo -e "${COLOR_BOLD}Installing SWIG ...${COLOR_RESET}"
if $AUTOINSTALL; then
	install_swig purge
else
	PKG_OK=$(dpkg-query -W --showformat='${Status}\n' dls-swig|grep "install ok installed")
	if [ "" != "$PKG_OK" ]; then
		echo -e -n "${COLOR_QUES}Do you want to re-install SWIG? [y/N]: ${COLOR_RESET}"
		read ANSWER_SWIG	
		if [ "$ANSWER_SWIG" == "Y" ] || [ "$ANSWER_SWIG" == "y" ]; then
			install_swig purge
		fi
	else
		echo -e -n "${COLOR_QUES}Do you want to install SWIG? [y/N]: ${COLOR_RESET}"
		read ANSWER_SWIG
		if [ "$ANSWER_SWIG" == "Y" ] || [ "$ANSWER_SWIG" == "y" ]; then
			install_swig
		fi
	fi
fi

##---------------------------------------------------------------##
##---------------------- Installing Ipopt -----------------------##
##---------------------------------------------------------------##
echo ""
echo -e "${COLOR_BOLD}Installing Ipopt ...${COLOR_RESET}"
if $AUTOINSTALL; then
	install_ipopt	purge
else		
	PKG_OK=$(dpkg-query -W --showformat='${Status}\n' dls-libipopt|grep "install ok installed")
	if [ "" != "$PKG_OK" ]; then
		echo -e -n "${COLOR_QUES}Do you want to re-install Ipopt? [y/N]: ${COLOR_RESET}"
		read ANSWER_IPOPT
		if [ "$ANSWER_IPOPT" == "Y" ] || [ "$ANSWER_IPOPT" == "y" ]; then
			install_ipopt purge
	    fi
	else
		echo -e -n "${COLOR_QUES}Do you want to install Ipopt? [y/N]: ${COLOR_RESET}"
		read ANSWER_IPOPT
		if [ "$ANSWER_IPOPT" == "Y" ] || [ "$ANSWER_IPOPT" == "y" ]; then
			install_ipopt
		fi
	fi
fi

##---------------------------------------------------------------##
##--------------------- Installing qpOASES ----------------------##
##---------------------------------------------------------------##
echo ""
echo -e "${COLOR_BOLD}Installing qpOASES ...${COLOR_RESET}"
if $AUTOINSTALL; then
	install_qpoases purge
else
	PKG_OK=$(dpkg-query -W --showformat='${Status}\n' dls-qpoases|grep "install ok installed")
	if [ "" != "$PKG_OK" ]; then
		echo -e -n "${COLOR_QUES}Do you want to re-install qpOASES? [y/N]: ${COLOR_RESET}"
		read ANSWER_QPOASES
		if [ "$ANSWER_QPOASES" == "Y" ] || [ "$ANSWER_QPOASES" == "y" ]; then
			install_qpoases purge
		fi
	else
		echo -e -n "${COLOR_QUES}Do you want to install qpOASES? [y/N]: ${COLOR_RESET}"
		read ANSWER_QPOASES
		if [ "$ANSWER_QPOASES" == "Y" ] || [ "$ANSWER_QPOASES" == "y" ]; then
			install_qpoases
		fi
	fi
fi

##---------------------------------------------------------------##
##--------------------- Installing CMA-ES -----------------------##
##---------------------------------------------------------------##
echo ""
echo -e "${COLOR_BOLD}Installing libcmaes ...${COLOR_RESET}"
if $AUTOINSTALL; then
	install_libcmaes purge
else
	PKG_OK=$(dpkg-query -W --showformat='${Status}\n' dls-libcmaes|grep "install ok installed")
	if [ "" != "$PKG_OK" ]; then
		echo -e -n "${COLOR_QUES}Do you want to re-install libcmaes? [y/N]: ${COLOR_RESET}"
		read ANSWER_LIBCMAES
		if [ "$ANSWER_LIBCMAES" == "Y" ] || [ "$ANSWER_LIBCMAES" == "y" ]; then
			install_libcmaes purge
		fi
	else
		echo -e -n "${COLOR_QUES}Do you want to install libcmaes? [y/N]: ${COLOR_RESET}"
		read ANSWER_LIBCMAES
		if [ "$ANSWER_LIBCMAES" == "Y" ] || [ "$ANSWER_LIBCMAES" == "y" ]; then
			install_libcmaes
		fi
	fi
fi

##---------------------------------------------------------------##
##-------------------- Installing PyADOLC -----------------------##
##---------------------------------------------------------------##
echo ""
echo -e "${COLOR_BOLD}Installing pyadolc ...${COLOR_RESET}"
if $AUTOINSTALL; then
	install_pyadolc purge
else
	PKG_OK=$(dpkg-query -W --showformat='${Status}\n' dls-pyadolc|grep "install ok installed")
	if [ "" != "$PKG_OK" ]; then
		echo -e -n "${COLOR_QUES}Do you want to re-install pyadolc? [y/N]: ${COLOR_RESET}"
		read ANSWER_PYADOLC
		if [ "$ANSWER_PYADOLC" == "Y" ] || [ "$ANSWER_PYADOLC" == "y" ]; then
			install_pyadolc purge
		fi
	else
		echo -e -n "${COLOR_QUES}Do you want to install pyadolc? [y/N]: ${COLOR_RESET}"
		read ANSWER_PYADOLC
		if [ "$ANSWER_PYADOLC" == "Y" ] || [ "$ANSWER_PYADOLC" == "y" ]; then
			install_pyadolc
		fi
	fi
fi

