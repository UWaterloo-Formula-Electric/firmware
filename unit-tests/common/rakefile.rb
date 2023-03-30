HERE = __dir__ + '/'

require 'rake'
require 'rake/clean'
require 'rake/testtask'
require './rakefile_helper'

include RakefileHelpers

REQUIRED_DIRS = ['./build', './build/mocks'].freeze
REQUIRED_DIRS.each do |v|
  directory v
end

# Load default configuration, for now
COMMON_CONFIG_FILE = 'common.yml'.freeze


task :unit do
end

task :bmu do
  BOARD_CONFIG_FILE = '../bmu/test.yml'
  configure_toolchain(COMMON_CONFIG_FILE, BOARD_CONFIG_FILE)
  run_tests(unit_test_files)
end

task :dcu do
  BOARD_CONFIG_FILE = '../dcu/test.yml'
  configure_toolchain(COMMON_CONFIG_FILE, BOARD_CONFIG_FILE)
  run_tests(unit_test_files)
end

task :pdu do
  BOARD_CONFIG_FILE = '../pdu/test.yml'
  configure_toolchain(COMMON_CONFIG_FILE, BOARD_CONFIG_FILE)
  run_tests(unit_test_files)
end

task :vcu do
  BOARD_CONFIG_FILE = '../vcu/test.yml'
  configure_toolchain(COMMON_CONFIG_FILE, BOARD_CONFIG_FILE)
  run_tests(unit_test_files)
end

desc 'Generate test summary'
task :summary do
  report_summary
end

desc 'Build and test Unity'
task :all => %i[clean unit summary]
task :default => REQUIRED_DIRS + %i[clobber all]
task :ci => [:default]
task :cruise => [:default]


desc 'Return error on Failures'
task :strict do
  $return_error_on_failures = true
end
