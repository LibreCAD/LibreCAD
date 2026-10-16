# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 LibreCAD.org
# Copyright (C) 2026 Dongxu Li

require 'fileutils'
require 'json'
require 'open3'
require 'tmpdir'
require 'yaml'

root = File.expand_path('../..', __dir__)
recipe = YAML.load_file(File.join(root, 'snapcraft.yaml'))
part = recipe.fetch('parts').fetch('librecad')
abort 'Snap must build the checkout and adopt its version' unless
  part.fetch('source') == '.' && recipe.fetch('adopt-info') == 'librecad' && !recipe.key?('version')
steps = YAML.load_file(File.join(root, '.github/workflows/build-all.yml'))
            .fetch('jobs').fetch('ReleaseAssets').fetch('steps')
kind = steps.find { |step| step['id'] == 'relkind' }
publish = steps.find { |step| step['uses'].to_s.start_with?('softprops/') }
abort 'Release action must receive the draft output' unless
  publish.fetch('with').fetch('draft') == '${{ steps.relkind.outputs.draft }}'
# Match the runner's distinction between unspecified and explicit Bash shells.
shell = case kind['shell']
        when nil then ['bash', '-e']
        when 'bash' then ['bash', '--noprofile', '--norc', '-eo', 'pipefail']
        else abort "Unsupported release shell: #{kind['shell']}"
        end

def run(environment, directory, *command)
  output, error, status = Open3.capture3(environment, *command, chdir: directory)
  abort "#{command.join(' ')} failed: #{output}\n#{error}" unless status.success?
  output.strip
end

Dir.mktmpdir('librecad-packaging-') do |directory|
  bin = File.join(directory, 'bin')
  FileUtils.mkdir_p(bin)
  File.write(File.join(bin, 'gh'), <<~BASH)
    #!/usr/bin/env bash
    set -eu
    [[ "$1" == api && "$2" == --paginate ]]
    printf '%s' "${MOCK_RELEASES:-}"
    exit "${MOCK_STATUS:-0}"
  BASH
  File.write(File.join(bin, 'craftctl'), <<~BASH)
    #!/usr/bin/env bash
    set -eu
    case "$*" in
      default) ;;
      'set version='*) printf '%s\n' "${2#version=}" > "$VERSION_LOG" ;;
      *) exit 2 ;;
    esac
  BASH
  FileUtils.chmod(0755, Dir.glob(File.join(bin, '*')))
  environment = {'PATH' => bin + ':' + ENV.fetch('PATH'), 'GIT_CONFIG_GLOBAL' => '/dev/null',
                 'GIT_CONFIG_NOSYSTEM' => '1', 'GITHUB_REPOSITORY' => 'LibreCAD/LibreCAD',
                 'GITHUB_OUTPUT' => File.join(directory, 'outputs'), 'VERSION_LOG' => File.join(directory, 'version')}

  cases = [
    ['stable draft', 'v2.2.1.6', [[{tag_name: 'v2.2.1.6', draft: true}]], 'true', 'false', 'false'],
    ['published release', 'v2.2.1.6', [[{tag_name: 'v2.2.1.6', draft: false}]], 'false', 'false', 'false'],
    ['new stable release', 'v2.2.1.6', [[]], 'false', 'true', 'false'],
    ['paginated RC draft', '2.2.1.6_rc3', [[{tag_name: 'v2.2.1.5', draft: false}],
                                       [{tag_name: '2.2.1.6_rc3', draft: true}]], 'true', 'false', 'true'],
    ['new hyphenated RC', '2.2.1.6-rc1', [[]], 'false', 'true', 'true']
  ]
  cases.each do |label, tag, pages, draft, notes, prerelease|
    FileUtils.rm_f(environment.fetch('GITHUB_OUTPUT'))
    env = environment.merge('GITHUB_REF' => "refs/tags/#{tag}", 'MOCK_STATUS' => '0',
                            'MOCK_RELEASES' => pages.map { |page| JSON.generate(page) }.join("\n"))
    run(env, directory, *shell, '-c', kind.fetch('run'))
    actual = File.readlines(environment.fetch('GITHUB_OUTPUT'), chomp: true).map { |line| line.split('=', 2) }.to_h
    expected = {'tag' => tag, 'draft' => draft, 'generate_notes' => notes, 'prerelease' => prerelease}
    abort "#{label}: #{actual} != #{expected}" unless actual == expected
    puts "PASS: #{label}"
  end
  [['API failure', '', '7'], ['partial pagination failure', '[]', '7'],
   ['invalid API response', '{', '0']].each do |label, response, exit_code|
    FileUtils.rm_f(environment.fetch('GITHUB_OUTPUT'))
    env = environment.merge('GITHUB_REF' => 'refs/tags/v2.2.1.6', 'MOCK_RELEASES' => response, 'MOCK_STATUS' => exit_code)
    _, _, status = Open3.capture3(env, *shell, '-c', kind.fetch('run'), chdir: directory)
    outputs = File.read(environment.fetch('GITHUB_OUTPUT'))
    abort "#{label}: release lookup did not fail closed" if status.success? || outputs.include?('draft=false')
    puts "PASS: #{label} stops publication"
  end

  checkout = File.join(directory, 'checkout')
  FileUtils.mkdir_p(File.join(checkout, 'librecad/src'))
  %w[CMakeLists.txt librecad/src/src.pro].each do |file|
    FileUtils.cp(File.join(root, file), File.join(checkout, file))
  end
  git = lambda { |*args| run(environment, checkout, 'git', *args) }
  git.call('init', '-b', '2.2.1')
  git.call('config', 'user.name', 'Packaging Test')
  git.call('config', 'user.email', 'packaging@example.invalid')
  git.call('add', '.')
  git.call('commit', '-m', 'baseline')
  git.call('tag', '-a', 'v2.2.1.5', '-m', 'baseline')
  check_version = lambda do |project, expected, label|
    FileUtils.rm_f(environment.fetch('VERSION_LOG'))
    run(environment.merge('CRAFT_PROJECT_DIR' => project), project, 'bash', '-eu', '-c', part.fetch('override-pull'))
    actual = File.read(environment.fetch('VERSION_LOG')).strip
    valid = /\A[a-zA-Z0-9](?:[a-zA-Z0-9:.+~\-]{0,30}[a-zA-Z0-9+~])?\z/.match?(actual)
    abort "#{label}: invalid or unexpected version #{actual}, expected #{expected}" unless actual == expected && valid
    puts "PASS: #{label} -> #{actual}"
  end
  check_version.call(checkout, '2.2.1.5', 'annotated stable tag')
  git.call('commit', '--allow-empty', '-m', 'branch update')
  expected = git.call('describe', '--always').delete_prefix('v')
  check_version.call(checkout, expected, 'branch commit after tag')
  git.call('-c', 'tag.gpgSign=false', 'tag', 'v9.9.9-lightweight')
  check_version.call(checkout, expected, 'lightweight tag is ignored')
  git.call('switch', '-c', 'unrelated')
  git.call('commit', '--allow-empty', '-m', 'unrelated release')
  git.call('tag', '-a', 'v9.9.9', '-m', 'unrelated release')
  git.call('switch', '2.2.1')
  check_version.call(checkout, expected, 'unrelated branch tag is ignored')

  remote = File.join(directory, 'remote.git')
  git.call('init', '--bare', remote)
  git.call('remote', 'add', 'origin', remote)
  [['2.2.1.6', 'stable', 'v2.2.1.6'], ['2.2.1.7', 'rc', '2.2.1.7_rc1'],
   ['2.2.1.8', 'rc3', '2.2.1.8_rc3']].each do |version, type, tag|
    run(environment, checkout, 'bash', File.join(root, 'scripts/release_github.sh'),
        version, type, '--no-push', '--yes')
    abort 'Release script did not create the annotated tag' unless git.call('describe', '--always') == tag &&
      git.call('cat-file', '-t', "refs/tags/#{tag}") == 'tag'
    abort 'Release script did not bump both source versions' unless
      File.read(File.join(checkout, 'librecad/src/src.pro')).include?("LC_VERSION=\"#{version}\"") &&
      File.read(File.join(checkout, 'CMakeLists.txt')).include?("add_compile_definitions(LC_VERSION=#{version})")
    check_version.call(checkout, tag.delete_prefix('v').tr('_', '-'), "release script #{type}")
  end
  abort 'Release test unexpectedly pushed' unless git.call('--git-dir', remote, 'for-each-ref').empty?
  archive = File.join(directory, 'archive/librecad/src')
  FileUtils.mkdir_p(archive)
  pro = File.read(File.join(root, 'librecad/src/src.pro'))
  File.write(File.join(archive, 'src.pro'), pro)
  check_version.call(File.join(directory, 'archive'), pro[/^LC_VERSION="([^"]*)"/, 1], 'source archive fallback')
end
