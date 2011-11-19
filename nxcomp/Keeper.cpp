/**************************************************************************/
/*                                                                        */
/* Copyright (c) 2001, 2010 NoMachine, http://www.nomachine.com/.         */
/*                                                                        */
/* NXCOMP, NX protocol compression and NX extensions to this software     */
/* are copyright of NoMachine. Redistribution and use of the present      */
/* software is allowed according to terms specified in the file LICENSE   */
/* which comes in the source distribution.                                */
/*                                                                        */
/* Check http://www.nomachine.com/licensing.html for applicability.       */
/*                                                                        */
/* NX and NoMachine are trademarks of Medialogic S.p.A.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "Keeper.h"

//
// Set the verbosity level.
//

#define PANIC
#define WARNING
#undef  TEST
#undef  DEBUG

//
// Remove the directory if it's empty
// since more than 30 * 24 h.
//

#define EMPTY_DIR_TIME  2592000

//
// Sleep once any ONCE entries.
//

#define ONCE  2

//
// Define this to trace messages while
// they are allocated and deallocated.
//

#undef REFERENCES

//
// This is used for reference count.
//

#ifdef REFERENCES

int File::references_ = 0;

#endif

bool T_older::operator () (File *a, File *b) const
{
  return a -> compare(b);
}

File::File()
{
  name_ = NULL;

  size_ = 0;
  time_ = 0;

  #ifdef REFERENCES

  references_++;

  *logofs << "Keeper: Created new File at "
          << this << " out of " << references_
          << " allocated references.\n"
          << logofs_flush;

  #endif
}

//
// TODO: This class can leak industrial amounts of memory.
// I'm 100% sure that the desctructor is called and that
// also the string pointed in the File structure is dele-
// ted. Everything is logged, but still the memory is not
// freed. This is less a problem on Windows, where the me-
// mory occupation remains almost constant. Obviously the
// problem lies in the STL allocators of the GNU libstc++.
//

File::~File()
{
  #ifdef TEST
  *logofs << "Keeper: Deleting name for File at "
          << this << ".\n" << logofs_flush;
  #endif

  delete [] name_;

  #ifdef REFERENCES

  *logofs << "Keeper: Deleted File at " 
          << this << " out of " << references_ 
          << " allocated references.\n"
          << logofs_flush;

  references_--;

  #endif
}

bool File::compare(File *b) const
{
  if (this -> time_ == b -> time_)
  {
    return (this -> size_ < b -> size_);
  }

  return (this -> time_ < b -> time_);
}

Keeper::Keeper(int caches, int images, const char *root,
                   int sleep, int parent)
{
  caches_ = caches;
  images_ = images;
  sleep_  = sleep;
  parent_ = parent;

  root_ = new char[strlen(root) + 1];

  strcpy(root_, root);

  total_  = 0;
  signal_ = 0;

  files_ = new T_files;
}

Keeper::~Keeper()
{
  empty();

  delete files_;

  delete [] root_;
}

int Keeper::cleanupCaches()
{
  #ifdef TEST
  *logofs << "Keeper: Looking for cache directories in '"
          << root_ << "'.\n" << logofs_flush;
  #endif

  DIR *rootDir = opendir(root_);

  if (rootDir != NULL)
  {
    dirent *dirEntry;

    struct stat fileStat;

    int baseSize = strlen(root_);

    int s = 0;

    while (((dirEntry = readdir(rootDir)) != NULL))
    {
      if (s++ % ONCE == 0) usleep(sleep_ * 1000);

      if (signal_ != 0) break;

      if (strcmp(dirEntry -> d_name, "cache") == 0 ||
              strncmp(dirEntry -> d_name, "cache-", 6) == 0)
      {
        char *dirName = new char[baseSize + strlen(dirEntry -> d_name) + 2];

        if (dirName == NULL)
        {
          #ifdef WARNING
          *logofs << "Keeper: WARNING! Can't check directory entry '"
                  << dirEntry -> d_name << "'.\n" << logofs_flush;
          #endif

          delete [] dirName;

          continue;
        }

        strcpy(dirName, root_);
        strcpy(dirName + baseSize, "/");
        strcpy(dirName + baseSize + 1, dirEntry -> d_name);

        #ifdef TEST
        *logofs << "Keeper: Checking directory '" << dirName
                << "'.\n" << logofs_flush;
        #endif

        if (stat(dirName, &fileStat) == 0 &&
                S_ISDIR(fileStat.st_mode) != 0)
        {
          //
          // Add to repository all the "C-" and
          // "S-" files in the given directory.
          //

          collect(dirName);
        }

        delete [] dirName;
      }
    }

    closedir(rootDir);
  }
  else
  {
    #ifdef WARNING
    *logofs << "Keeper: WARNING! Can't open NX root directory '"
            << root_ << "'. Error is " << EGET() << " '"
            << ESTR() << "'.\n" << logofs_flush;
     #endif

     cerr << "Warning" << ": Can't open NX root directory '"
          << root_ << "'. Error is " << EGET() << " '"
          << ESTR() << "'.\n";
  }

  //
  // Remove older files.
  //

  cleanup(caches_);

  //
  // Empty the repository.
  //

  empty();

  return 1;
}

int Keeper::cleanupImages()
{
  #ifdef TEST
  *logofs << "Keeper: Looking for image directory in '"
          << root_ << "'.\n" << logofs_flush;
  #endif

  char *imagesPath = new char[strlen(root_) + strlen("/images") + 1];

  if (imagesPath == NULL)
  {
    return -1;
  }

  strcpy(imagesPath, root_);
  strcat(imagesPath, "/images");

  //
  // Check if the cache directory does exist.
  //

  struct stat dirStat;

  if (stat(imagesPath, &dirStat) == -1)
  {
    #ifdef WARNING
    *logofs << "Keeper: WARNING! Can't stat NX images cache directory '"
            << imagesPath << ". Error is " << EGET() << " '"
            << ESTR() << "'.\n" << logofs_flush;
    #endif

    cerr << "Warning" << ": Can't stat NX images cache directory '"
         << imagesPath << ". Error is " << EGET() << " '"
         << ESTR() << "'.\n";

    delete [] imagesPath;

    return -1;
  }

  //
  // Check any of the 16 directories in the
  // images root path.
  //

  char *digitPath = new char[strlen(imagesPath) + 5];

  strcpy(digitPath, imagesPath);

  for (char digit = 0; digit < 16; digit++)
  {
    //
    // Give up if we received a signal or
    // our parent is gone.
    //

    if (signal_ != 0)
    {
      #ifdef TEST
      *logofs << "Keeper: Signal detected. Aborting.\n"
              << logofs_flush;
      #endif

      goto KeeperCleanupImagesAbort;
    }
    else if (parent_ != getppid() || parent_ == 1)
    {
      #ifdef WARNING
      *logofs << "Keeper: WARNING! Parent process appears "
              << "to be dead. Returning.\n"
              << logofs_flush;
      #endif

      goto KeeperCleanupImagesAbort;

      return 0;
    }

    sprintf(digitPath + strlen(imagesPath), "/I-%01X", digit);

    //
    // Add to the repository all the files
    // in the given directory.
    //

    collect(digitPath);
  }

  delete [] imagesPath;
  delete [] digitPath;

  //
  // Remove the oldest files.
  //

  cleanup(images_);

  //
  // Empty the repository.
  //

  empty();

  return 1;

KeeperCleanupImagesAbort:

  delete [] imagesPath;
  delete [] digitPath;

  empty();

  return 0;
}

int Keeper::collect(const char *path)
{
  #ifdef TEST
  *logofs << "Keeper: Looking for files in directory '"
          << path << "'.\n" << logofs_flush;
  #endif

  DIR *cacheDir = opendir(path);

  if (cacheDir != NULL)
  {
    File *file;

    dirent *dirEntry;

    struct stat fileStat;

    int baseSize = strlen(path);
    int fileSize = baseSize + 3 + MD5_LENGTH * 2 + 1;

    int n = 0;
    int s = 0;

    while (((dirEntry = readdir(cacheDir)) != NULL))
    {
      if (s++ % ONCE == 0) usleep(sleep_ * 1000);

      if (signal_ != 0) break;

      if (strcmp(dirEntry -> d_name, ".") == 0 ||
              strcmp(dirEntry -> d_name, "..") == 0)
      {
        continue;
      }      

      n++;

      if (strlen(dirEntry -> d_name) == (MD5_LENGTH * 2 + 2) &&
              (strncmp(dirEntry -> d_name, "I-", 2) == 0 ||
                  strncmp(dirEntry -> d_name, "S-", 2) == 0 ||
                      strncmp(dirEntry -> d_name, "C-", 2) == 0))
      {
        file = new File();

        char *fileName = new char[fileSize];

        if (file == NULL || fileName == NULL)
        {
          #ifdef WARNING
          *logofs << "Keeper: WARNING! Can't add file '"
                  << dirEntry -> d_name << "' to repository.\n"
                  << logofs_flush;
          #endif

          delete [] fileName;

          delete file;

          continue;
        }

        strcpy(fileName, path);
        strcpy(fileName + baseSize, "/");
        strcpy(fileName + baseSize + 1, dirEntry -> d_name);

        file -> name_ = fileName;

        #ifdef DEBUG
        *logofs << "Keeper: Adding file '" << file -> name_
                << "'.\n" << logofs_flush;
        #endif

        if (stat(file -> name_, &fileStat) == -1)
        {
          #ifdef WARNING
          *logofs << "Keeper: WARNING! Can't stat NX file '"
                  << file -> name_ << ". Error is " << EGET()
                  << " '" << ESTR() << "'.\n"
                  << logofs_flush;
          #endif

          delete file;

          continue;
        }

        file -> size_ = fileStat.st_size;
        file -> time_ = fileStat.st_mtime;

        files_ -> insert(T_files::value_type(file));

        total_ += file -> size_;
      }
    }

    closedir(cacheDir);

    if (n == 0)
    {
      time_t now = time(NULL);

      if (now > 0 && stat(path, &fileStat) == 0)
      {
        #ifdef TEST
        *logofs << "Keeper: Empty NX subdirectory '" << path
                << "' unused since " << now - fileStat.st_mtime
                << " S.\n" << logofs_flush;
        #endif

        if (now - fileStat.st_mtime > EMPTY_DIR_TIME)
        {
          #ifdef TEST
          *logofs << "Keeper: Removing empty NX subdirectory '"
                  << path << "'.\n" << logofs_flush;
          #endif

          rmdir(path);
        }
      }
    }
  }
  else
  {
    #ifdef WARNING
    *logofs << "Keeper: WARNING! Can't open NX subdirectory '"
            << path << ". Error is " << EGET() << " '" << ESTR()
            << "'.\n" << logofs_flush;
     #endif

     cerr << "Warning" << ": Can't open NX subdirectory '"
          << path << ". Error is " << EGET() << " '" << ESTR()
          << "'.\n";
  }

  return 1;
}

int Keeper::cleanup(int threshold)
{
  #ifdef TEST
  *logofs << "Keeper: Deleting the oldest files with "
          << files_ -> size() << " elements threshold "
          << threshold << " and size " << total_ << ".\n"
          << logofs_flush;
  #endif

  //
  // At least some versions of the standard
  // library don't allow erasing an element
  // while looping. This is not the most ef-
  // ficient way to release the objects, but
  // it's better than making a copy of the
  // container.
  //

  while (total_ > threshold && files_ -> size() > 0)
  {
    T_files::iterator i = files_ -> begin();

    File *file = *i;

    #ifdef TEST
    *logofs << "Keeper: Removing '" << file -> name_
            << "' with time " << file -> time_ << " and size "
            << file -> size_ << ".\n" << logofs_flush;
    #endif

    unlink(file -> name_);

    total_ -= file -> size_;

    #ifdef DEBUG
    *logofs << "Keeper: Going to delete the file at "
            << file << " while cleaning up.\n"
            << logofs_flush;
    #endif

    delete file;

    #ifdef DEBUG
    *logofs << "Keeper: Going to erase the element "
            << "while cleaning up.\n"
            << logofs_flush;
    #endif

    files_ -> erase(i);
  }

  #ifdef TEST
  *logofs << "Keeper: Bytes in repository are "
          << total_ << ".\n" << logofs_flush;
  #endif

  return 1;
}

void Keeper::empty()
{
  #ifdef TEST
  *logofs << "Keeper: Getting rid of files in repository.\n"
          << logofs_flush;
  #endif

  while (files_ -> size() > 0)
  {
    T_files::iterator i = files_ -> begin();

    File *file = *i;

    #ifdef DEBUG
    *logofs << "Keeper: Going to delete the file at "
            << file << " while emptying.\n"
            << logofs_flush;
    #endif

    delete file;

    #ifdef DEBUG
    *logofs << "Keeper: Going to erase the element "
            << "while emptying.\n"
            << logofs_flush;
    #endif

    files_ -> erase(i);
  }

  total_ = 0;

  #ifdef TEST
  *logofs << "Keeper: Bytes in repository are "
          << total_ << ".\n" << logofs_flush;
  #endif
}
