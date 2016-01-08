env = Environment(
  CC='clang++',
  CCFLAGS=Split('''
    -std=c++11
    -O3
    -c
  ''')
)

file=Split('/home/schurchill/ASIO/waltr/src/waltr.cpp')

libs=Split('''
  pthread
  protobuf
  libboost_chrono.a
  libboost_filesystem.a
  libboost_system.a
  libboost_program_options.a
''')

libpath=Split('''
  /usr/lib
  /boost/stage/lib
''')

env.Program( source=file, LIBS=libs, LIBPATH=libpath )
