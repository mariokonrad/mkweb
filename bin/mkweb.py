#!/usr/bin/python

import os
import sys
import json
import yaml
import collections
import datetime
import time
import subprocess
import tempfile
import shutil
import argparse


class Config:
	"""
	This class provides easy access to configuration. It also
	provides default values.
	"""

	def __init__(self, yaml_filename):
		with open(yaml_filename, 'r') as config_file:
			text = config_file.read()
			self.data = yaml.load(text)

	def get(self, field, default_value):
		if self.data and (field in self.data):
			return self.data[field]
		return default_value

	def get_source(self):
		return self.get('source', 'pages')

	def get_destination(self):
		return self.get('destination', 'public')

	def get_static(self):
		return self.get('static', None)

	def get_plugin_path(self, plugin):
		path = self.get('plugins', None)
		if path:
			path += '/' + plugin + '/'
		return path

	def get_site_url(self):
		return self.get('site_url', '?')

	def get_plugin_url(self, plugin):
		url = self.get('plugin_url', None)
		if url:
			url += plugin + '/'
		return url

	def get_site_title(self):
		return self.get('site_title', 'TITLE')

	def get_site_subtitle(self):
		return self.get('site_subtitle', None)

	def get_theme(self):
		return self.get('theme', 'default')

	def get_author(self):
		return self.get('author', '?')

	def get_num_news(self):
		return self.get('num_news', 8)

	def get_source_process_filetypes(self):
		return self.get('source-process_filetypes', ['.md'])

	def get_path_map(self):
		return self.get('path_map', [])

	def get_social_enable(self):
		return self.get('social-enable', False)

	def get_social(self):
		return self.get('social', '')

	def get_menu_enable(self):
		return self.get('menu-enable', False)

	def get_menu(self):
		return self.get('menu', '')

	def get_page_tags_enable(self):
		return self.get('page-tags-enable', False)

	def get_tags_enable(self):
		return self.get('tags-enable', False)

	def get_years_enable(self):
		return self.get('years-enable', False)

	def get_pagelist_enable(self):
		return self.get('pagelist-enable', False)

	def get_pagelist_sort_key(self):
		if self.data and ('pagelist-sort' in self.data):
			cfg = self.data['pagelist-sort']
			if 'key' in cfg:
				key = cfg['key']
				if key in ['title', 'date']:
					return key
		return 'title'

	def get_pagelist_sort_dir(self):
		if self.data and ('pagelist-sort' in self.data):
			cfg = self.data['pagelist-sort']
			if 'direction' in cfg:
				direction = cfg['direction']
				if direction in ['ascending', 'descending']:
					return direction
		return 'ascending'

#
# global data
#
config = None
global_taglist = u''
global_yearlist = u''
global_pagelist = u''
global_meta = {}
pandoc_bin = 'pandoc'


class System:
	@staticmethod
	def get_theme_path():
		return os.path.dirname(os.path.realpath(__file__)) + '/../shared/themes/' + config.get_theme() + '/'

	@staticmethod
	def get_plugin_path(plugin):
		return os.path.dirname(os.path.realpath(__file__)) + '/../shared/plugins/' + plugin + '/'

	@staticmethod
	def get_theme_template():
		return System.get_theme_path() + 'template.html'

	@staticmethod
	def get_theme_style():
		return System.get_theme_path() + 'style.html'

	@staticmethod
	def get_theme_footer():
		return System.get_theme_path() + 'footer.html'

	@staticmethod
	def get_plugin_style(plugin):
		return System.get_plugin_path(plugin) + 'style.html'


def copytree(src, dst, symlinks = False, ignore = None):
	"""
	from: http://stackoverflow.com/questions/1868714/how-do-i-copy-an-entire-directory-of-files-into-an-existing-directory-using-pyth
	because shutil.copytree is annoying regarding the existance of the destination directory.
	shutils.copytree calls mkdir and fails unncesseraly
	"""
	if not os.path.exists(dst):
		os.makedirs(dst)
		shutil.copystat(src, dst)
	lst = os.listdir(src)
	if ignore:
		excl = ignore(src, lst)
		lst = [x for x in lst if x not in excl]
	for item in lst:
		s = os.path.join(src, item)
		d = os.path.join(dst, item)
		if symlinks and os.path.islink(s):
			if os.path.lexists(d):
				os.remove(d)
			os.symlink(os.readlink(s), d)
			try:
				st = os.lstat(s)
				mode = stat.S_IMODE(st.st_mode)
				os.lchmod(d, mode)
			except:
				pass # lchmod not available
		elif os.path.isdir(s):
			copytree(s, d, symlinks, ignore)
		else:
			shutil.copy2(s, d)

def copy_plugin_files(plugin):
	print('install plugin: ' + plugin)
	plugin_path = config.get_plugin_path(plugin)
	with open(System.get_plugin_path(plugin) + 'files.yml', 'r') as config_file:
		cfg = yaml.load(config_file.read())
		if cfg and ('install' in cfg):
			for f in cfg['install']:
				fn = System.get_plugin_path(plugin) + f
				if os.path.exists(fn):
					if os.path.isfile(fn):
						print('  copy "'+ fn + '" -> "' + plugin_path + '"')
						ensure_path(plugin_path)
						shutil.copy2(fn, plugin_path)
					elif os.path.isdir(fn):
						print('  copy "'+ fn + '" -> "' + plugin_path + f + '"')
						ensure_path(plugin_path)
						copytree(fn, plugin_path + f)
					else:
						raise IOError('entity to copy is not a file or directory')

def make_exclude_file_func(file_types):
	"""
	Returns a function to filter configured file types. All the specified
	file types are collected and returned as an exclude list.
	"""
	def func(directory, files):
		exclude = []
		for filename in files:
			fn, ext = os.path.splitext(filename)
			if ext in file_types:
				exclude.append(filename)
		return exclude
	return func

def ensure_path(filename):
	"""
	if the directory does not exist, it will be atempted to create it.
	if the specified filename ends with '/', it will interpreted as directory.
	if it does not end with '/', it will be interpreted as file.
	"""
	path = os.path.dirname(filename)
	if not path:
		return True
	if os.path.exists(path):
		if os.path.isdir(path):
			return True
		else:
			return False
	os.makedirs(path)
	return True

def read_meta(path):
	fn, ext = os.path.splitext(path)
	if ext == '.md':
		# extract header from file, yaml format
		with open(path, 'r') as markdown_file:
			text = markdown_file.read()
			text = text.partition('---')[2].partition('---')[0]
			return yaml.load(text)
	return None

def collect_information(root_directory):
	"""
	collect information from files in regard to tags
	"""
	info = {}
	tags = {}
	years = {}
	plugins = set([])
	for dirname, subdirlist, filelist in os.walk(root_directory):
		for fn in filelist:
			filename = dirname + '/' + fn
			rel_filename = os.path.relpath(dirname, root_directory) + '/' + fn
			meta = read_meta(filename)

			# if a file does not provide metadata, there is nothing
			# we can do at this point
			if meta == None:
				continue

			# replace 'date' entry with datetime
			if 'date' in meta:
				meta['date'] = make_datetime(meta['date'])

			# read meta data and remember general file information
			info[rel_filename] = meta

			# collect plugins
			if 'plugins' in meta:
				for plugin in meta['plugins']:
					plugins.add(plugin)

			# check and extract tags
			if 'tags' in meta:
				for tag in meta['tags']:
					if not tag in tags:
						tags[tag] = []
					tags[tag].append({'filename':rel_filename, 'meta':meta})

			# check and extract date tags
			if 'date' in meta:
				d = meta['date']
				if d.year not in years:
					years[d.year] = []
				years[d.year].append({'filename':rel_filename, 'meta':meta})

	return info, tags, years, plugins

def replace_root(link):
	parts = link.split('/')
	if len(parts) < 2:
		return link
	for path in config.get_path_map():
		if path['base'] == parts[0]:
			if path['absolute']:
				return link.replace(parts[0] + '/', path['url'])
			else:
				return link.replace(parts[0] + '/', config.get_site_url() + path['url'])
	return link

def handle_link(data):
	if ('c' in data) and (type(data['c']) == list):
		data['c'][-1][0] = replace_root(data['c'][-1][0])

def recursive_search_links(data):
	if type(data) == list:
		for item in data:
			recursive_search_links(item)
	elif type(data) == dict:
		if ('t' in data) and (data['t'] == 'Para'):
			recursive_search_links(data['c'])
		elif ('t' in data) and (data['t'] == 'Link'):
			handle_link(data)
		elif ('t' in data) and (data['t'] == 'Image'):
			handle_link(data)
		else:
			for key in data:
				recursive_search_links(data[key])
	else:
		pass

def conversion_necessary(filename_in, filename_out):
	"""
	Checks the modification date of the output file against
	relevant input files (concrete input file, theme files).
	Changes in configuration are not detected.
	"""
	if not os.path.exists(filename_in):
		return False
	if not os.path.exists(filename_out):
		return True
	mtime_out = os.path.getmtime(filename_out)
	if mtime_out < os.path.getmtime(filename_in):
		return True
	if mtime_out < os.path.getmtime(System.get_theme_template()):
		return True
	if mtime_out < os.path.getmtime(System.get_theme_style()):
		return True
	if mtime_out < os.path.getmtime(System.get_theme_footer()):
		return True
	return False

def get_meta_for_source(filename_in):
	filename = filename_in.replace(config.get_source() + '/', '')
	if filename in global_meta:
		return global_meta[filename]
	return None

def make_plugin_script_string(plugin, filename):
	return u'<script type="text/javascript" src="' + config.get_plugin_url(plugin) \
		+ filename + '"></script>\n'

def create_header_for_plugin(plugin):
	s = u''
	with open(System.get_plugin_path(plugin) + 'files.yml', 'r') as config_file:
		cfg = yaml.load(config_file.read())
		if cfg and ('include' in cfg):
			for f in cfg['include']:
				s += make_plugin_script_string(plugin, f)
	return s

def process_document(filename_in, filename_out, pagetags = None):
	if not conversion_necessary(filename_in, filename_out):
		print('skip    %s' % (filename_out))
		return

	print('        %s' % (filename_out))
	ensure_path(filename_out)

	# conversion from source file to JSON

	params = [pandoc_bin, '-t', 'json', filename_in]
	text = subprocess.Popen(params, stdout = subprocess.PIPE).communicate()[0]

	# process JSON AST

	j = json.loads(text)
	recursive_search_links(j)

	# prepare final conversion parameter

	params = [pandoc_bin,
		'-f', 'json',
		'-t', 'html5',
		'-o', filename_out,
		'-H', System.get_theme_style(),
		'-A', System.get_theme_footer(),
		'-M', 'title-prefix=' + config.get_site_title(),
		'-V', 'siteurl=' + config.get_site_url(),
		'-V', 'sitetitle=' + config.get_site_title(),
		'--template', System.get_theme_template(),
		'--standalone',
		'--preserve-tabs',
		'--toc', '--toc-depth=2',
		'--mathml'
		]
	if config.get_site_subtitle() != None:
		params.extend(['-V', 'sitesubtitle=' + config.get_site_subtitle()])
	if config.get_tags_enable():
		params.extend(['-V', 'globaltags=' + global_taglist])
	if config.get_years_enable():
		params.extend(['-V', 'globalyears=' + global_yearlist])
	if config.get_social_enable():
		params.extend(['-V', 'social=' + config.get_social()])
	if config.get_menu_enable():
		params.extend(['-V', 'menu=' + config.get_menu()])
	if config.get_page_tags_enable() and pagetags:
		params.extend(['-V', 'pagetags='+ pagetags])
	if config.get_pagelist_enable() and (len(global_pagelist) > 0):
		params.extend(['-V', 'globalpagelist=' + global_pagelist])

	meta = get_meta_for_source(filename_in)
	if meta and ('plugins' in meta):
		for plugin in meta['plugins']:
			params.extend(['-H', System.get_plugin_style(plugin)])
			params.extend(['-V', 'header-string=' + create_header_for_plugin(plugin)])

	# execute final conversion to HTML

	p = subprocess.Popen(params,
		stdout = subprocess.PIPE,
		stdin = subprocess.PIPE,
		stderr = subprocess.PIPE)
	out, err = p.communicate(input=json.dumps(j))
	if err:
		raise IOError('error in executing pandoc to create html, err: ' + err)

def convert_path(path):
	"""
	Checks wheather or not the specified file is to be accepted, and returns
	the filename with its destination file extension. If the file is not accepted,
	None is returned.
	"""
	fn, ext = os.path.splitext(path)
	if ext in config.get_source_process_filetypes():
		return path.replace(ext, '.html')
	return None

def process_single(source_directory, destination_directory, path):
	filename_in = path
	filename_out = convert_path(filename_in)
	if filename_out:
		filename_out = filename_out.replace(source_directory, destination_directory)
		process_document(filename_in, filename_out, prepare_page_taglist(filename_in))
	else:
		print('ignore: %s' % (filename_in))

def is_subdir(path, directory):
	path = os.path.realpath(path)
	directory = os.path.realpath(directory)
	relative = os.path.relpath(path, directory)
	return not (relative == os.pardir or relative.startswith(os.pardir + os.sep))

def process_pages(source_directory, destination_directory, specific_dir = None):
	if specific_dir:
		if not is_subdir(specific_dir, source_directory):
			raise IOError('specified directory "%s" is not a subdirectory of "%s"' % (specific_dir, source_directory))
	for dirname, subdirlist, filelist in os.walk(source_directory):
		if specific_dir and (specific_dir != dirname):
			continue
		for fn in filelist:
			process_single(source_directory, destination_directory, dirname + '/' + fn)

def prepare_taglist(ids):
	s = '<ul>'
	for tag in sorted(ids):
		s += '<li><a href="'+config.get_site_url()+'tag/'+tag+'.html">'+tag+'</a></li>'
	s += '</ul>\n'
	return s

def prepare_page_taglist(filename_in):
	meta = get_meta_for_source(filename_in)
	if not meta:
		return None
	if not ('tags' in meta):
		return None
	ids = []
	for tag in meta['tags']:
		ids.append(tag)
	return prepare_taglist(ids)

def prepare_global_taglist(tags):
	ids = []
	for tag in tags:
		ids.append(tag)
	return prepare_taglist(ids)

def prepare_global_yearlist(years):
	ids = []
	for year in years:
		ids.append(str(year))
	s = u'<br>'
	for year in sorted(ids, reverse = True):
		s += '<a href="'+config.get_site_url()+'year/'+year+'.html">'+year+'</a> '
	return s

def prepare_global_pagelist():
	# extract sorting criteria, build map key->filename
	key = config.get_pagelist_sort_key()
	ids = []
	for fn in global_meta:
		if key in global_meta[fn]:
			ids.append({'key':global_meta[fn][key], 'filename':fn})

	# generate HTML list of sorted entries
	reverse_sort = config.get_pagelist_sort_dir() == 'descending'
	s = u'<ul>'
	for entry in sorted(ids, key = lambda x : x['key'], reverse = reverse_sort):
		filename = entry['filename']
		title = global_meta[filename]['title']
		s += '<li><a href="'+config.get_site_url()+convert_path(filename)+'">'+title+'</a></li>'
	s += '</ul>\n'
	return s

def process_tags(tags):
	"""
	creates processed tag files, containing links to all pages for the tags
	"""
	file_meta_info = """
---
title: "Tag Overview: %s"
author: %s
date: 2000-00-00
language: en
---
"""
	path = config.get_destination() + '/tag'
	ensure_path(path + '/')
	tmp = tempfile.mkdtemp()
	for tag in tags:
		filename = tmp + '/' + tag + '.md'
		try:
			f = open(filename, 'w+')
			f.write((file_meta_info % (tag, config.get_author())).encode('utf8'))
			for info in sorted(tags[tag], key = lambda x : x['meta']['title']):
				title = info['meta']['title']
				link = config.get_source() + '/' + info['filename'].replace('.md', '.html')
				f.write(('- [%s](%s)\n' % (title, link)).encode('utf8'))
			f.close()
		except:
			shutil.rmtree(tmp)
			raise IOError('error in processing tag files')
	process_pages(tmp, path)
	shutil.rmtree(tmp)

def process_years(years):
	"""
	creates processed year files, containing link to all pages for the years
	"""
	file_meta_info = """
---
title: "Year Overview: %s"
author: %s
date: 2000-00-00
language: en
---
"""
	path = config.get_destination() + '/year'
	ensure_path(path + '/')
	tmp = tempfile.mkdtemp()
	for year in years:
		filename = tmp + '/' + str(year) + '.md'
		try:
			f = open(filename, 'w+')
			f.write((file_meta_info % (year, config.get_author())).encode('utf8'))
			for info in sorted(years[year], key = lambda x : x['meta']['title']):
				title = info['meta']['title']
				link = config.get_source() + '/' + info['filename'].replace('.md', '.html')
				f.write(('- [%s](%s)\n' % (title, link)).encode('utf8'))
			f.close()
		except:
			shutil.rmtree(tmp)
			raise IOError('error in processing tag files')
	process_pages(tmp, path)
	shutil.rmtree(tmp)

def make_datetime(d):
	"""
	Returns a datetime object, constructed from the specified data
	"""
	if isinstance(d, datetime.datetime):
		return d
	if isinstance(d, datetime.date):
		return datetime.datetime(d.year, d.month, d.day, 0, 0, 0)
	if isinstance(d, str):
		t = time.strptime(d, "%Y-%m-%d %H:%M")
		return datetime.datetime(t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, 0)
	if isinstance(d, (int, long)):
		return datetime.datetime(d, 1, 1, 0, 0, 0)
	return datetime.datetime(2000, 1, 1, 0, 0, 0)

def process_front():
	"""
	creates front page
	"""
	file_meta_info = """
---
title: Contents
author: %s
date: 2016-01-01
language: en
---
"""

	# collect map date->filename
	dates = {}
	for filename in global_meta:
		d = make_datetime(None)
		if 'date' in global_meta[filename]:
			d = global_meta[filename]['date']
		if not d in dates:
			dates[d] = []
		dates[d].append(filename)
	sorted_dates = collections.OrderedDict(sorted(dates.items(), reverse=True))

	# dump newest entries
	tmp = tempfile.mkdtemp()
	try:
		index_filename = tmp + '/index.md'
		f = open(index_filename, 'w+')
		f.write((file_meta_info % (config.get_author())).encode('utf8'))
		f.write(('Newest Entries:\n\n').encode('utf8'));
		count = 0
		for d in sorted_dates:
			if count >= config.get_num_news():
				break
			for filename in sorted_dates[d]:
				if count >= config.get_num_news():
					break
				count += 1
				pretty_time = time.strftime("%Y-%m-%d", d.timetuple())
				title = global_meta[filename]['title']
				link = config.get_source() + '/' + filename.replace('.md', '.html')
				f.write(('  - `%s` : [%s](%s)\n' % (pretty_time, title, link)).encode('utf8'))
				if 'summary' in global_meta[filename]:
					f.write(('\n').encode('utf8'));
					f.write(('    %s\n' % (global_meta[filename]['summary'])).encode('utf8'))
				f.write(('\n').encode('utf8'));
		f.close()
	except:
		shutil.rmtree(tmp)
		raise IOError('error in creating front')
	ensure_path(config.get_destination())
	process_document(index_filename, config.get_destination() + '/index.html')
	shutil.rmtree(tmp)

def process_redirect(directory):
	"""
	adds redirection to all subdirectories which have no `index.html`
	"""

	redirect_contents = """
<!DOCTYPE html>
<html><head><meta http-equiv="refresh" content="0;url=%s"></head><body></body></html>
"""

	for dirname, subdirlist, filelist in os.walk(directory):
		filename = dirname + '/index.html'
		if not os.path.exists(filename):
			print('redir:  %s' % (filename))
			try:
				with open(filename, 'w+') as redirect_file:
					redirect_file.write((redirect_contents % (config.get_site_url())).encode('utf8'))
			except:
				pass

def main(arg = None):
	global config
	global global_taglist
	global global_yearlist
	global global_pagelist
	global global_meta
	global pandoc_bin

	# parse parameters
	parser = argparse.ArgumentParser('Static Website Generator')
	parser.add_argument('--config',
		nargs = 1,
		default = ['config.yml'],
		help = 'Specify a configuration file')
	parser.add_argument('--file',
		nargs = 1,
		help = 'Specify a file or directory to process. This file or directory must be a part of the configured source directory within the configuration file.')
	parser.add_argument('--copy',
		action = 'store_true',
		help = 'Copies files from "static" to "destination".')
	parser.add_argument('--plugins',
		action = 'store_true',
		help = 'Copies plugin files.')
	parser.add_argument('--pandoc',
		nargs = 1,
		help = 'path to the pandoc binary')
	args = parser.parse_args()

	# validate stuff
	if not os.path.exists(args.config[0]):
		raise IOError('file not readable: ' + args.config[0])
	if args.file and len(args.file) > 1:
		raise IOError('only one file or directory allowed')
	if args.pandoc:
		if not os.path.exists(args.pandoc[0]):
			raise IOError('specified pandoc binary does not exist: ' + args.pandoc[0])
		pandoc_bin = args.pandoc[0]

	# change directory to the configuration file, because it is possible to
	# specify relative paths in the configuration file. this way both relative
	# and absolute paths within the configuration file work fine.
	config_filename = args.config[0]
	if os.path.dirname(config_filename):
		os.chdir(os.path.dirname(config_filename))

	# read configuration
	config = Config(config_filename)

	# collect information
	global_meta, tags, years, plugins = collect_information(config.get_source())

	# render global lists
	global_taglist = prepare_global_taglist(tags)
	global_yearlist = prepare_global_yearlist(years)
	global_pagelist = prepare_global_pagelist()

	# generate site
	if args.file:
		path = args.file[0]
		if not os.path.exists(path):
			raise IOError('specified file or directory does not exist')
		if os.path.isdir(path):
			process_pages(config.get_source(), config.get_destination(), specific_dir = path)
		else:
			process_single(config.get_source(), config.get_destination(), path)
	else:
		process_tags(tags)
		process_years(years)
		process_pages(config.get_source(), config.get_destination())
		process_front()
		process_redirect(config.get_destination())
		args.copy = True
		args.plugins = True

	# copy files
	if args.copy:
		print('copy files')

		# if no static directory is configured, we assume the source directory to
		# contain files to copy. Just let's ignore the file types we are already
		# processing.
		if config.get_static() and (config.get_static() != config.get_source()):
			copytree(config.get_static(), config.get_destination())
		else:
			copytree(config.get_source(), config.get_destination(),
				ignore = make_exclude_file_func(config.get_source_process_filetypes()))

	# copy plugins
	if args.plugins:
		print('copy plugins')
		for plugin in plugins:
			copy_plugin_files(plugin)

if __name__ == '__main__':
	status = main()
	sys.exit(status)

