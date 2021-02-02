/*
    This file is part of amake.

    amake is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    amake is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with amake.  If not, see <https://www.gnu.org/licenses/>.
 * */

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>

struct SrcFile
{
	const static unsigned char	C_FILE = 0x00,
					CPP_FILE = 0x01;
	unsigned char	type;
	std::string	name;
};

typedef std::list<SrcFile>	SrcFileList;
typedef std::list<std::string>	StringList;

static std::string get_extension(const char *file)
{
	const char *p = strrchr(file, '.');
	if (!p) return "";
	const int len = strlen(file);
	return std::string(p+1, len - (p+1-file));
}

static std::string get_deps(const char *file)
{
	const std::string 	TMP(".tmp.dep");
	std::stringstream 	sstr,
				out;
	std::string		ret;
	
	sstr << "g++ -std=c++0x -MM " << file << " > " << TMP;
	if(system(sstr.str().c_str()))
		throw std::runtime_error((std::string("Can't execute '") + sstr.str() + "'").c_str());
	
	int 	fp = open(TMP.c_str(), O_RDONLY);
	char	buf[1024];
	int	rb = 0;
	if (fp > -1)
	{
		while((rb = read(fp, buf, 1024)) > 0)
			out.write(buf, rb);
		ret = out.str();
		close(fp);
	}
	remove(TMP.c_str());
	if (!ret.empty() && *(ret.rbegin()) == '\n')
		ret.resize(ret.size()-1);
	return ret;
}

static void search_dir(const char *base, SrcFileList& out)
{
	out.clear();
	DIR *dirRef = opendir(base);
	if (dirRef)
	{
		struct dirent *curFile = NULL;
		while((curFile = readdir(dirRef)))
		{
			if ('.' == curFile->d_name[0] ||
				!strcmp(".", curFile->d_name) ||
				!strcmp("..", curFile->d_name))
				continue;
			std::string ext = get_extension(curFile->d_name);
			if (!strcasecmp(ext.c_str(), "c"))
			{
				SrcFile ent;
				ent.type = SrcFile::C_FILE;
				ent.name = std::string(base) + "/" + curFile->d_name;
				out.push_back(ent);
			}
			else if(!strcasecmp(ext.c_str(), "cpp") ||
				!strcasecmp(ext.c_str(), "cc"))
			{
				SrcFile ent;
				ent.type = SrcFile::CPP_FILE;
				ent.name = std::string(base) + "/" + curFile->d_name;
				out.push_back(ent);
			}
		}
		closedir(dirRef);
	}
}

static void get_o_list(const SrcFileList& in, StringList& out)
{
	out.clear();
	for(SrcFileList::const_iterator it = in.begin(); it != in.end(); ++it)
	{
		char	ofile[256];
		if (it->name.size() > 255) throw "File name too large";
		strcpy(ofile, it->name.c_str());
		char *ptr = strrchr(ofile, '.');
		if (ptr)
		{
			ptr[1] = 'o';
			ptr[2] = '\0';
			char *ptr =strrchr(ofile, '/');
			if (ptr) ++ptr;
			else ptr = ofile;
			out.push_back(std::string("$(OBJDIR)/") + ptr);
		}
	}
}

static void print_logo(void)
{
	std::cout << "amake - Simple Makefile generator" << std::endl;
	std::cout << "\t(C) Emanuele Oriani 2021\n" << std::endl;
}

static void print_help(void)
{
	std::cout << "Usage: amake [--help] [-src SRCDIR] [-o OUTNAME] [gcc/g++ options] [gcc/g++ libraries]" << std::endl;
	std::cout << "\t--help : display this help" << std::endl;
	std::cout << "\t-src : set the sources and headers directory" << std::endl;
	std::cout << "\t-o : set the name for the compiled file" << std::endl;
	std::cout << "\tgcc/g++ options : set any option you want to pass to the compiler" << std::endl;
	std::cout << "\tgcc/g++ libraries : set any library you want to pass to the compiler" << std::endl;
}

int main(int argc, char *argv[])
{
	try
	{
		SrcFileList 	fl;
		StringList	o_list,
				lib_list,
				flag_list;
		std::string	exec_name("out"),
				objdir("obj"),
				srcdir(".");
		time_t		curTime = time(NULL);
        	
		for(int i = 1; i < argc; ++i)
		{
			if (strlen(argv[i]) >= 3 && argv[i][0] == '-' && (argv[i][1] == 'l' || argv[i][1] == 'L'))
				lib_list.push_back(argv[i]);
			else if (strlen(argv[i]) == 2 && argv[i][0] == '-' && argv[i][1] == 'o')
			{
				if (i+1 < argc)
				{
					exec_name = std::string(argv[i+1]);
					++i;
				}
			}
			else if (!strcasecmp(argv[i], "-src"))
			{
				if (i+1 < argc)
				{
					srcdir = std::string(argv[i+1]);
					++i;
				}
			}
			else if (!strcasecmp(argv[i], "--help"))
			{
				print_logo();
				print_help();
				return 0;
			}
			else flag_list.push_back(argv[i]);
		}
		search_dir(srcdir.c_str(), fl);
		get_o_list(fl, o_list);

		bool	link_gcc = true;
		for(SrcFileList::const_iterator it = fl.begin(); it != fl.end(); ++it)
			if(it->type == SrcFile::CPP_FILE) {
				link_gcc = false;
				break;
			}
		
		std::ofstream of("Makefile");
		of << "#Makefile generated by amake\n#On " << ctime(&curTime);
		of << "#To print amake help use \'amake --help\'." << std::endl;
		of << "CC=gcc" << std::endl;
		of << "CPPC=g++" << std::endl;
		of << "LINK=" << (link_gcc ? "gcc" : "g++") << std::endl;
		of << "SRCDIR=" << srcdir << std::endl;
		of << "OBJDIR=" << objdir << std::endl;
		of << "FLAGS=-g -Wall ";
		for(StringList::const_iterator it = flag_list.begin(); it != flag_list.end(); ++it)
			of << *it << " ";
		of << std::endl;
		of << "LIBS=";
		for(StringList::const_iterator it = lib_list.begin(); it != lib_list.end(); ++it)
			of << *it << " ";
		of << std::endl;
		of << "OBJS=";
		for(StringList::const_iterator it = o_list.begin(); it != o_list.end(); ++it)
			of << *it << " ";
		of << std::endl;
		of << "EXEC=" << exec_name << std::endl;
		of << "DATE=$(shell date +\"%Y-%m-%d\")" << std::endl;
		of << std::endl;
	
		of << "$(EXEC) : $(OBJS)" << std::endl;
		of << '\t' << "$(LINK) $(OBJS) -o $(EXEC) ";
		of << "$(FLAGS) $(LIBS)\n" << std::endl;
	
		for(SrcFileList::const_iterator it = fl.begin(); it != fl.end(); ++it)
		{
			of << "$(OBJDIR)/" << get_deps(it->name.c_str()) << " $(OBJDIR)/__setup_obj_dir" << std::endl;
			of << '\t';
			if (it->type == SrcFile::C_FILE)
				of << "$(CC) ";
			else if (it->type == SrcFile::CPP_FILE)
				of << "$(CPPC) ";
			else throw "Invalid source file";
			of << "$(FLAGS) ";
			of << it->name.c_str() << " ";
			of << "-c ";
			of << "-o $@";
			of << '\n' << std::endl;		
		}

		of << "$(OBJDIR)/__setup_obj_dir :" << std::endl;
		of << '\t' << "mkdir -p $(OBJDIR)" << std::endl;
		of << '\t' << "touch $(OBJDIR)/__setup_obj_dir" << std::endl;
		of << std::endl;

		of << ".PHONY: clean bzip release" << std::endl;
		of << std::endl;

		of << "clean :" << std::endl;
		of << '\t' << "rm -rf $(OBJDIR)/*.o" << std::endl;
		of << '\t' << "rm -rf $(EXEC)" << std::endl;
		of << std::endl;

		of << "bzip :" << std::endl;
		of << '\t' << "tar -cvf \"$(DATE).$(EXEC).tar\" $(SRCDIR)/* Makefile" << std::endl;
		of << '\t' << "bzip2 \"$(DATE).$(EXEC).tar\"" << std::endl;
		of << std::endl;

		of << "release : FLAGS +=-O3 -D_RELEASE" << std::endl;
		of << "release : $(EXEC)" << std::endl;
		of << std::endl;
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception : " << e.what() << std::endl;
	}
	catch(char *exc)
	{
		std::cerr << "Exception : " << exc << std::endl;
	}
	catch(...)
	{
		std::cerr << "Unknown exception" << std::endl;
	}
	
	return 0;
}
