#!/usr/bin/perl -w
#
# $XdotOrg: xc/programs/Xserver/hw/xfree86/os-support/sunos/find_deps.pl,v 1.2 2004/09/22 17:20:56 alanc Exp $
#
# Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, and/or sell copies of the Software, and to permit persons
# to whom the Software is furnished to do so, provided that the above
# copyright notice(s) and this permission notice appear in all copies of
# the Software and that both the above copyright notice(s) and this
# permission notice appear in supporting documentation.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
# OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
# INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
# FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
# WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
# 
# Except as contained in this notice, the name of a copyright holder
# shall not be used in advertising or otherwise to promote the sale, use
# or other dealings in this Software without prior written authorization
# of the copyright holder.
#
#------------------------------------------------------------------------------
#
# This script scans the X server, it's libraries, and shared object drivers
# and generates the linker flags necessary to allow the shared object modules 
# to load via dlopen().  
#
# WARNING:  Do not try this at home boys and girls!  Only trained professionals
# should try this stunt.  This script is not intended to serve as an example of
# proper use of the linker or associated tools, but merely as an unfortunately
# necessary bit of hackery to get Xserver modules to load via Solaris dlopen 
# instead of the XFree86 custom loader/runtime linker.
#
# No guarantee of usability or suitablity is made - in fact it's almost 
# guaranteed this is not suitable for any other use, and maybe not even
# for the one it was intended.
#
# Usage: find_deps.pl [-R ProjectRoot] <paths_to_scan>
# Expects to be called while cwd is a directory containing Xorg or XFree86
# server binary.

use strict;
use File::Find;
use Getopt::Std;

my @objlist = ();
my %symtable = ();

my $servername;

my $ProjectRoot = "/usr/X11R6";

my %opts;
getopts('R:', \%opts);
if (exists($opts{"R"})) {
  $ProjectRoot = $opts{"R"};
}

if (-f "Xorg") {
  $servername = "Xorg";
} elsif (-f "XFree86") {
  $servername = "XFree86";
} else {
  die "Cannot find X server";
}

$File::Find::name = $servername;
$_  = $servername;
&scanobjs;

find({wanted => \&scanobjs, preprocess => \&filterobjs}, @ARGV);

for my $f (@objlist) {
  open(ELFDUMP, "/usr/ccs/bin/elfdump -r $f|") || die "Cannot open file";
  my $edline;
  my %deps = ();
  while ($edline = <ELFDUMP>) {
    next unless $edline =~ /(GLOB_DAT|R_386_32|R_SPARC_32)/;
    my @edpart = split /\s+/, $edline;
    if (exists $symtable{$edpart[$#edpart]}) {
#      print "$f : $edpart[$#edpart] - $symtable{$edpart[$#edpart]}\n";
      $deps{$symtable{$edpart[$#edpart]}} += 1;
    } else {
      print "$f : $edpart[$#edpart] - not found\n";
    }
  }
  close(ELFDUMP);

  my $depslist = "";

  for my $d (sort keys %deps) {
    next if ($f =~ /$d/ || $d !~ /\.so$/);
      $depslist .= " -Wl,-N,$d";
  }
  print "$f : $depslist\n";
  my $depsfile = $f ."_deps";
  if ($depslist ne "") {
    my $dirlist = "-R $ProjectRoot/lib/modules";
    if ($f =~ /drivers/) {
      $dirlist .= " -R $ProjectRoot/lib/modules/drivers";
    }
    if ($depslist =~ /libfbdevhw.so/) {
      $dirlist .= " -R $ProjectRoot/lib/modules/linux/";
    }
    if ($depslist =~ /libGLcore.so/) {
      $dirlist .= " -R $ProjectRoot/lib/modules/extensions/";
    }
    
    open(MODDEPSFILE, '>', $depsfile) || die "Cannot write to $depsfile";
    print MODDEPSFILE $dirlist, $depslist, "\n";
    close(MODDEPSFILE);
  } elsif (! -z $depsfile) {
    unlink($depsfile);
    system("touch $depsfile");
  }
}


sub filterobjs {
  return (grep( ($_ =~ /\.so$/) || (-d $_) , @_));
}

sub scanobjs {
  return if /libXfont.so/;
  return if (-d $_);
  print "Scanning $File::Find::name ...\n";
  push @objlist, $File::Find::name;
  open(NMOUT, "/usr/ccs/bin/nm $_|") || die "Cannot nm file $_";
  my $nmline;
  while ($nmline = <NMOUT>) {
    next unless $nmline =~ /\|/;
    my @nmpart = split(/\s*\|\s*/, $nmline);
    next unless ($nmpart[4] eq "GLOB") && ($nmpart[6] ne "UNDEF");
    chomp($nmpart[7]);
    if (! exists $symtable{$nmpart[7]}) {
      $symtable{$nmpart[7]} = $_;
    }
  }
  close(NMOUT);
}
