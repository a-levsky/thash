# thash
NAME
       thash - A multithreaded file hashing program.

SYNOPSIS
       thash -a  ALGORITHMS [-f  FILE] [-e  FILE] [-o  FILE] [-t  NUM] FILE...

DESCRIPTION
       thash will calculate and output file hashes from a list of files.

       By  default  thash  calculates the hash value (algorithm to be determined by the value of -a switch) of a file or files specified on
       the command line. thash then outputs, to standard output, each file's name (with absolute path) along with a comma separated list of
       hash values for that file. The command's behaviour can be modified through the use of the switches and arguments in the OPTIONS sec‐
       tion below.

       thash is multithreaded and uses a producer/consumer model for quick and efficient hashing calculations.  The main process (the  pro‐
       ducer)  populates  a  queue  of  file names to be hashed. A number of consumer threads (2 by default) read file names (actually, the
       absolute path to a file) from the queue and calculate the hash value of those files and print the values to standard output.

OPTIONS
       -a ALGORITHMS, --algorithms=ALGORITHMS
              Hash the files using any of the following hashing algorithms, separated by commas: md5, sha1,  sha256,  sha512.  See EXAMPLES
              section for sample usage.

       -f FILE, --file=FILE
              File paths of files to be hashed are to be read, one per line, from the specified FILE.

       -e FILE, --error=FILE
              Output errors to FILE.

       -o FILE, --output=FILE
              Output the comma separated list of file names and hashes to FILE.

       -t NUM, --threads=NUM
              Try to process the file hashing queue using NUM consumer threads.

EXAMPLES
       Hash two files specified on the command line using md5 and outputting errors to a file.
              thash -a md5 -e errors.txt ~/myfile1 ~/myfile2
              
       Hash a list of files read from a text file using sha1 and sha512 using 2 threads.
              thash -a sha1,sha512 -f ~/filelist.txt -t 2

       Hash a list of files read from a file using md5 and sha1, using 4 threads and writing output to a file.
              thash -a md5,sha1 -f ~/inputfile -o ~/outputfile -t 4

SAMPLE EXECUTION
       $ thash -a md5,sha1 -f ~/myfiles.txt -t 2

       /home/student/rick.txt,d9de48cf0676e9edb99bd8ee1ed44a21,8dd98a90abbb563294e771288c5484cc887faef6

       /home/student/morty.mp3,5a52a1d0458c40d92c91090437796f75,b5a5b36b6be8d98c6f1bea655536d67abef23be8


