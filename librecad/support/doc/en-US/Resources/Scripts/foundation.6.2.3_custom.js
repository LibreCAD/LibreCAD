/*
 * Foundation Responsive Library
 * http://foundation.zurb.com
 * Copyright 2017, ZURB
 * Free to use under the MIT license.
 * http://www.opensource.org/licenses/mit-license.php
*/

!function ($) {

    "use strict";

    var FOUNDATION_VERSION = '6.2.3';

    // Global Foundation object
    // This is attached to the window, or used as a module for AMD/Browserify
    var Foundation = {
        version: FOUNDATION_VERSION,

        /**
         * Stores initialized plugins.
         */
        _plugins: {},

        /**
         * Stores generated unique ids for plugin instances
         */
        _uuids: [],

        /**
         * Returns a boolean for RTL support
         */
        rtl: function () {
            return $('html').attr('dir') === 'rtl';
        },
        /**
         * Defines a Foundation plugin, adding it to the `Foundation` namespace and the list of plugins to initialize when reflowing.
         * @param {Object} plugin - The constructor of the plugin.
         */
        plugin: function (plugin, name) {
            // Object key to use when adding to global Foundation object
            // Examples: Foundation.Reveal, Foundation.OffCanvas
            var className = name || functionName(plugin);
            // Object key to use when storing the plugin, also used to create the identifying data attribute for the plugin
            // Examples: data-reveal, data-off-canvas
            var attrName = hyphenate(className);

            // Add to the Foundation object and the plugins list (for reflowing)
            this._plugins[attrName] = this[className] = plugin;
        },
        /**
         * @function
         * Populates the _uuids array with pointers to each individual plugin instance.
         * Adds the `zfPlugin` data-attribute to programmatically created plugins to allow use of $(selector).foundation(method) calls.
         * Also fires the initialization event for each plugin, consolidating repetitive code.
         * @param {Object} plugin - an instance of a plugin, usually `this` in context.
         * @param {String} name - the name of the plugin, passed as a camelCased string.
         * @fires Plugin#init
         */
        registerPlugin: function (plugin, name) {
            var pluginName = name ? hyphenate(name) : functionName(plugin.constructor).toLowerCase();
            plugin.uuid = this.GetYoDigits(6, pluginName);

            if (!plugin.$element.attr('data-' + pluginName)) {
                plugin.$element.attr('data-' + pluginName, plugin.uuid);
            }
            if (!plugin.$element.data('zfPlugin')) {
                plugin.$element.data('zfPlugin', plugin);
            }
            /**
             * Fires when the plugin has initialized.
             * @event Plugin#init
             */
            plugin.$element.trigger('init.zf.' + pluginName);

            this._uuids.push(plugin.uuid);

            return;
        },
        /**
         * @function
         * Removes the plugins uuid from the _uuids array.
         * Removes the zfPlugin data attribute, as well as the data-plugin-name attribute.
         * Also fires the destroyed event for the plugin, consolidating repetitive code.
         * @param {Object} plugin - an instance of a plugin, usually `this` in context.
         * @fires Plugin#destroyed
         */
        unregisterPlugin: function (plugin) {
            var pluginName = hyphenate(functionName(plugin.$element.data('zfPlugin').constructor));

            this._uuids.splice(this._uuids.indexOf(plugin.uuid), 1);
            plugin.$element.removeAttr('data-' + pluginName).removeData('zfPlugin')
                /**
                 * Fires when the plugin has been destroyed.
                 * @event Plugin#destroyed
                 */
                .trigger('destroyed.zf.' + pluginName);
            for (var prop in plugin) {
                plugin[prop] = null; //clean up script to prep for garbage collection.
            }
            return;
        },

        /**
         * @function
         * Causes one or more active plugins to re-initialize, resetting event listeners, recalculating positions, etc.
         * @param {String} plugins - optional string of an individual plugin key, attained by calling `$(element).data('pluginName')`, or string of a plugin class i.e. `'dropdown'`
         * @default If no argument is passed, reflow all currently active plugins.
         */
        reInit: function (plugins) {
            var isJQ = plugins instanceof $;
            try {
                if (isJQ) {
                    plugins.each(function () {
                        $(this).data('zfPlugin')._init();
                    });
                } else {
                    var type = typeof plugins,
                        _this = this,
                        fns = {
                            'object': function (plgs) {
                                plgs.forEach(function (p) {
                                    p = hyphenate(p);
                                    $('[data-' + p + ']').foundation('_init');
                                });
                            },
                            'string': function () {
                                plugins = hyphenate(plugins);
                                $('[data-' + plugins + ']').foundation('_init');
                            },
                            'undefined': function () {
                                this['object'](Object.keys(_this._plugins));
                            }
                        };
                    fns[type](plugins);
                }
            } catch (err) {
                console.error(err);
            } finally {
                return plugins;
            }
        },

        /**
         * returns a random base-36 uid with namespacing
         * @function
         * @param {Number} length - number of random base-36 digits desired. Increase for more random strings.
         * @param {String} namespace - name of plugin to be incorporated in uid, optional.
         * @default {String} '' - if no plugin name is provided, nothing is appended to the uid.
         * @returns {String} - unique id
         */
        GetYoDigits: function (length, namespace) {
            const randomBuffer = new Uint32Array(1);
            var crypto = window.crypto || window.msCrypto;
            crypto.getRandomValues(randomBuffer);
            let randomNumber = randomBuffer[0] / (0xffffffff + 1);

            length = length || 6;
            return Math.round(Math.pow(36, length + 1) - randomNumber * Math.pow(36, length)).toString(36).slice(1) + (namespace ? '-' + namespace : '');
        },
        /**
         * Initialize plugins on any elements within `elem` (and `elem` itself) that aren't already initialized.
         * @param {Object} elem - jQuery object containing the element to check inside. Also checks the element itself, unless it's the `document` object.
         * @param {String|Array} plugins - A list of plugins to initialize. Leave this out to initialize everything.
         */
        reflow: function (elem, plugins) {

            // If plugins is undefined, just grab everything
            if (typeof plugins === 'undefined') {
                plugins = Object.keys(this._plugins);
            }
            // If plugins is a string, convert it to an array with one item
            else if (typeof plugins === 'string') {
                plugins = [plugins];
            }

            var _this = this;

            // Iterate through each plugin
            $.each(plugins, function (i, name) {
                // Get the current plugin
                var plugin = _this._plugins[name];

                // Localize the search to all elements inside elem, as well as elem itself, unless elem === document
                var $elem = $(elem).find('[data-' + name + ']').addBack('[data-' + name + ']');

                // For each plugin found, initialize it
                $elem.each(function () {
                    var $el = $(this),
                        opts = {};
                    // Don't double-dip on plugins
                    if ($el.data('zfPlugin')) {
                        console.warn("Tried to initialize " + name + " on an element that already has a Foundation plugin.");
                        return;
                    }

                    if ($el.attr('data-options')) {
                        var thing = $el.attr('data-options').split(';').forEach(function (e, i) {
                            var opt = e.split(':').map(function (el) {
                                return el.trim();
                            });
                            if (opt[0]) opts[opt[0]] = parseValue(opt[1]);
                        });
                    }
                    try {
                        $el.data('zfPlugin', new plugin($(this), opts));
                    } catch (er) {
                        console.error(er);
                    } finally {
                        return;
                    }
                });
            });
        },
        getFnName: functionName,
        transitionend: function ($elem) {
            var transitions = {
                'transition': 'transitionend',
                'WebkitTransition': 'webkitTransitionEnd',
                'MozTransition': 'transitionend',
                'OTransition': 'otransitionend'
            };
            var elem = document.createElement('div'),
                end;

            for (var t in transitions) {
                if (typeof elem.style[t] !== 'undefined') {
                    end = transitions[t];
                }
            }
            if (end) {
                return end;
            } else {
                end = setTimeout(function () {
                    $elem.triggerHandler('transitionend', [$elem]);
                }, 1);
                return 'transitionend';
            }
        }
    };

    Foundation.util = {
        /**
         * Function for applying a debounce effect to a function call.
         * @function
         * @param {Function} func - Function to be called at end of timeout.
         * @param {Number} delay - Time in ms to delay the call of `func`.
         * @returns function
         */
        throttle: function (func, delay) {
            var timer = null;

            return function () {
                var context = this,
                    args = arguments;

                if (timer === null) {
                    timer = setTimeout(function () {
                        func.apply(context, args);
                        timer = null;
                    }, delay);
                }
            };
        }
    };

    // TODO: consider not making this a jQuery function
    // TODO: need way to reflow vs. re-initialize
    /**
     * The Foundation jQuery method.
     * @param {String|Array} method - An action to perform on the current jQuery object.
     */
    var foundation = function (method) {
        var type = typeof method,
            $meta = $('meta.foundation-mq'),
            $noJS = $('.no-js');

        if (!$meta.length) {
            $('<meta class="foundation-mq">').appendTo(document.head);
        }
        if ($noJS.length) {
            $noJS.removeClass('no-js');
        }

        if (type === 'undefined') {
            //needs to initialize the Foundation object, or an individual plugin.
            Foundation.MediaQuery._init();
            Foundation.reflow(this);
        } else if (type === 'string') {
            //an individual method to invoke on a plugin or group of plugins
            var args = Array.prototype.slice.call(arguments, 1); //collect all the arguments, if necessary
            var plugClass = this.data('zfPlugin'); //determine the class of plugin

            if (plugClass !== undefined && plugClass[method] !== undefined) {
                //make sure both the class and method exist
                if (this.length === 1) {
                    //if there's only one, call it directly.
                    plugClass[method].apply(plugClass, args);
                } else {
                    this.each(function (i, el) {
                        //otherwise loop through the jQuery collection and invoke the method on each
                        plugClass[method].apply($(el).data('zfPlugin'), args);
                    });
                }
            } else {
                //error for no class or no method
                throw new ReferenceError("We're sorry, '" + method + "' is not an available method for " + (plugClass ? functionName(plugClass) : 'this element') + '.');
            }
        } else {
            //error for invalid argument type
            throw new TypeError('We\'re sorry, ' + type + ' is not a valid parameter. You must use a string representing the method you wish to invoke.');
        }
        return this;
    };

    window.Foundation = Foundation;
    $.fn.foundation = foundation;

    // Polyfill for requestAnimationFrame
    (function () {
        if (!Date.now || !window.Date.now) window.Date.now = Date.now = function () {
            return new Date().getTime();
        };

        var vendors = ['webkit', 'moz'];
        for (var i = 0; i < vendors.length && !window.requestAnimationFrame; ++i) {
            var vp = vendors[i];
            window.requestAnimationFrame = window[vp + 'RequestAnimationFrame'];
            window.cancelAnimationFrame = window[vp + 'CancelAnimationFrame'] || window[vp + 'CancelRequestAnimationFrame'];
        }
        if (/iP(ad|hone|od).*OS 6/.test(window.navigator.userAgent) || !window.requestAnimationFrame || !window.cancelAnimationFrame) {
            var lastTime = 0;
            window.requestAnimationFrame = function (callback) {
                var now = Date.now();
                var nextTime = Math.max(lastTime + 16, now);
                return setTimeout(function () {
                    callback(lastTime = nextTime);
                }, nextTime - now);
            };
            window.cancelAnimationFrame = clearTimeout;
        }
        /**
         * Polyfill for performance.now, required by rAF
         */
        if (!window.performance || !window.performance.now) {
            window.performance = {
                start: Date.now(),
                now: function () {
                    return Date.now() - this.start;
                }
            };
        }
    })();
    if (!Function.prototype.bind) {
        Function.prototype.bind = function (oThis) {
            if (typeof this !== 'function') {
                // closest thing possible to the ECMAScript 5
                // internal IsCallable function
                throw new TypeError('Function.prototype.bind - what is trying to be bound is not callable');
            }

            var aArgs = Array.prototype.slice.call(arguments, 1),
                fToBind = this,
                fNOP = function () {},
                fBound = function () {
                    return fToBind.apply(this instanceof fNOP ? this : oThis, aArgs.concat(Array.prototype.slice.call(arguments)));
                };

            if (this.prototype) {
                // native functions don't have a prototype
                fNOP.prototype = this.prototype;
            }
            fBound.prototype = new fNOP();

            return fBound;
        };
    }
    // Polyfill to get the name of a function in IE9
    function functionName(fn) {
        if (Function.prototype.name === undefined) {
            var funcNameRegex = /function\s([^(]{1,})\(/;
            var results = funcNameRegex.exec(fn.toString());
            return results && results.length > 1 ? results[1].trim() : "";
        } else if (fn.prototype === undefined) {
            return fn.constructor.name;
        } else {
            return fn.prototype.constructor.name;
        }
    }
    function parseValue(str) {
        if (/true/.test(str)) return true;else if (/false/.test(str)) return false;else if (!isNaN(str * 1)) return parseFloat(str);
        return str;
    }
    // Convert PascalCase to kebab-case
    // Thank you: http://stackoverflow.com/a/8955580
    function hyphenate(str) {
        return str.replace(/([a-z])([A-Z])/g, '$1-$2').toLowerCase();
    }
}(jQuery);
'use strict';

!function ($) {

    // Default set of media queries
    var defaultQueries = {
        'default': 'only screen',
        landscape: 'only screen and (orientation: landscape)',
        portrait: 'only screen and (orientation: portrait)',
        retina: 'only screen and (-webkit-min-device-pixel-ratio: 2),' + 'only screen and (min--moz-device-pixel-ratio: 2),' + 'only screen and (-o-min-device-pixel-ratio: 2/1),' + 'only screen and (min-device-pixel-ratio: 2),' + 'only screen and (min-resolution: 192dpi),' + 'only screen and (min-resolution: 2dppx)'
    };

    var MediaQuery = {
        queries: [],

        current: '',

        /**
         * Initializes the media query helper, by extracting the breakpoint list from the CSS and activating the breakpoint watcher.
         * @function
         * @private
         */
        _init: function () {
            var self = this;
            var extractedStyles = $('.foundation-mq').css('font-family');
            var namedQueries;

            namedQueries = parseStyleToObject(extractedStyles);

            for (var key in namedQueries) {
                if (namedQueries.hasOwnProperty(key)) {
                    self.queries.push({
                        name: key,
                        value: 'only screen and (min-width: ' + namedQueries[key] + ')'
                    });
                }
            }

            this.current = this._getCurrentSize();

            this._watcher();
        },


        /**
         * Checks if the screen is at least as wide as a breakpoint.
         * @function
         * @param {String} size - Name of the breakpoint to check.
         * @returns {Boolean} `true` if the breakpoint matches, `false` if it's smaller.
         */
        atLeast: function (size) {
            var query = this.get(size);

            if (query) {
                return window.matchMedia(query).matches;
            }

            return false;
        },


        /**
         * Gets the media query of a breakpoint.
         * @function
         * @param {String} size - Name of the breakpoint to get.
         * @returns {String|null} - The media query of the breakpoint, or `null` if the breakpoint doesn't exist.
         */
        get: function (size) {
            for (var i in this.queries) {
                if (this.queries.hasOwnProperty(i)) {
                    var query = this.queries[i];
                    if (size === query.name) return query.value;
                }
            }

            return null;
        },


        /**
         * Gets the current breakpoint name by testing every breakpoint and returning the last one to match (the biggest one).
         * @function
         * @private
         * @returns {String} Name of the current breakpoint.
         */
        _getCurrentSize: function () {
            var matched;

            for (var i = 0; i < this.queries.length; i++) {
                var query = this.queries[i];

                if (window.matchMedia(query.value).matches) {
                    matched = query;
                }
            }

            if (typeof matched === 'object') {
                return matched.name;
            } else {
                return matched;
            }
        },


        /**
         * Activates the breakpoint watcher, which fires an event on the window whenever the breakpoint changes.
         * @function
         * @private
         */
        _watcher: function () {
            var _this = this;

            $(window).on('resize.zf.mediaquery', function () {
                var newSize = _this._getCurrentSize(),
                    currentSize = _this.current;

                if (newSize !== currentSize) {
                    // Change the current media query
                    _this.current = newSize;

                    // Broadcast the media query change on the window
                    $(window).trigger('changed.zf.mediaquery', [newSize, currentSize]);
                }
            });
        }
    };

    Foundation.MediaQuery = MediaQuery;

    // matchMedia() polyfill - Test a CSS media type/query in JS.
    // Authors & copyright (c) 2012: Scott Jehl, Paul Irish, Nicholas Zakas, David Knight. Dual MIT/BSD license
    window.matchMedia || (window.matchMedia = function () {
        'use strict';

        // For browsers that support matchMedium api such as IE 9 and webkit

        var styleMedia = window.styleMedia || window.media;

        // For those that don't support matchMedium
        if (!styleMedia) {
            var style = document.createElement('style'),
                script = document.getElementsByTagName('script')[0],
                info = null;

            style.type = 'text/css';
            style.id = 'matchmediajs-test';

            script.parentNode.insertBefore(style, script);

            // 'style.currentStyle' is used by IE <= 8 and 'window.getComputedStyle' for all other browsers
            info = 'getComputedStyle' in window && window.getComputedStyle(style, null) || style.currentStyle;

            styleMedia = {
                matchMedium: function (media) {
                    var text = '@media ' + media + '{ #matchmediajs-test { width: 1px; } }';

                    // 'style.styleSheet' is used by IE <= 8 and 'style.textContent' for all other browsers
                    if (style.styleSheet) {
                        style.styleSheet.cssText = text;
                    } else {
                        style.textContent = text;
                    }

                    // Test if media query is true or false
                    return info.width === '1px';
                }
            };
        }

        return function (media) {
            return {
                matches: styleMedia.matchMedium(media || 'all'),
                media: media || 'all'
            };
        };
    }());

    // Thank you: https://github.com/sindresorhus/query-string
    function parseStyleToObject(str) {
        var styleObject = {};

        if (typeof str !== 'string') {
            return styleObject;
        }

        str = str.trim().slice(1, -1); // browsers re-quote string style values

        if (!str) {
            return styleObject;
        }

        styleObject = str.split('&').reduce(function (ret, param) {
            var parts = param.replace(/\+/g, ' ').split('=');
            var key = parts[0];
            var val = parts[1];
            key = decodeURIComponent(key);

            // missing `=` should be `null`:
            // http://w3.org/TR/2012/WD-url-20120524/#collect-url-parameters
            val = val === undefined ? null : decodeURIComponent(val);

            if (!ret.hasOwnProperty(key)) {
                ret[key] = val;
            } else if (Array.isArray(ret[key])) {
                ret[key].push(val);
            } else {
                ret[key] = [ret[key], val];
            }
            return ret;
        }, {});

        return styleObject;
    }

    Foundation.MediaQuery = MediaQuery;
}(jQuery);
/*******************************************
 *                                         *
 * This util was created by Marius Olbertz *
 * Please thank Marius on GitHub /owlbertz *
 * or the web http://www.mariusolbertz.de/ *
 *                                         *
 ******************************************/

'use strict';

!function ($) {

    var keyCodes = {
        9: 'TAB',
        13: 'ENTER',
        27: 'ESCAPE',
        32: 'SPACE',
        37: 'ARROW_LEFT',
        38: 'ARROW_UP',
        39: 'ARROW_RIGHT',
        40: 'ARROW_DOWN'
    };

    var commands = {};

    var Keyboard = {
        keys: getKeyCodes(keyCodes),

        /**
         * Parses the (keyboard) event and returns a String that represents its key
         * Can be used like Foundation.parseKey(event) === Foundation.keys.SPACE
         * @param {Event} event - the event generated by the event handler
         * @return String key - String that represents the key pressed
         */
        parseKey: function (event) {
            var key = keyCodes[event.which || event.keyCode] || String.fromCharCode(event.which).toUpperCase();
            if (event.shiftKey) key = 'SHIFT_' + key;
            if (event.ctrlKey) key = 'CTRL_' + key;
            if (event.altKey) key = 'ALT_' + key;
            return key;
        },


        /**
         * Handles the given (keyboard) event
         * @param {Event} event - the event generated by the event handler
         * @param {String} component - Foundation component's name, e.g. Slider or Reveal
         * @param {Objects} functions - collection of functions that are to be executed
         */
        handleKey: function (event, component, functions) {
            var commandList = commands[component],
                keyCode = this.parseKey(event),
                cmds,
                command,
                fn;

            if (!commandList) return console.warn('Component not defined!');

            if (typeof commandList.ltr === 'undefined') {
                // this component does not differentiate between ltr and rtl
                cmds = commandList; // use plain list
            } else {
                // merge ltr and rtl: if document is rtl, rtl overwrites ltr and vice versa
                if (Foundation.rtl()) cmds = $.extend({}, commandList.ltr, commandList.rtl);else cmds = $.extend({}, commandList.rtl, commandList.ltr);
            }
            command = cmds[keyCode];

            fn = functions[command];
            if (fn && typeof fn === 'function') {
                // execute function  if exists
                var returnValue = fn.apply();
                if (functions.handled || typeof functions.handled === 'function') {
                    // execute function when event was handled
                    functions.handled(returnValue);
                }
            } else {
                if (functions.unhandled || typeof functions.unhandled === 'function') {
                    // execute function when event was not handled
                    functions.unhandled();
                }
            }
        },


        /**
         * Finds all focusable elements within the given `$element`
         * @param {jQuery} $element - jQuery object to search within
         * @return {jQuery} $focusable - all focusable elements within `$element`
         */
        findFocusable: function ($element) {
            return $element.find('a[href], area[href], input:not([disabled]), select:not([disabled]), textarea:not([disabled]), button:not([disabled]), iframe, object, embed, *[tabindex], *[contenteditable]').filter(function () {
                if (!$(this).is(':visible') || $(this).attr('tabindex') < 0) {
                    return false;
                } //only have visible elements and those that have a tabindex greater or equal 0
                return true;
            });
        },


        /**
         * Returns the component name name
         * @param {Object} component - Foundation component, e.g. Slider or Reveal
         * @return String componentName
         */

        register: function (componentName, cmds) {
            commands[componentName] = cmds;
        }
    };

    /*
     * Constants for easier comparing.
     * Can be used like Foundation.parseKey(event) === Foundation.keys.SPACE
     */
    function getKeyCodes(kcs) {
        var k = {};
        for (var kc in kcs) {
            k[kcs[kc]] = kcs[kc];
        }return k;
    }

    Foundation.Keyboard = Keyboard;
}(jQuery);
'use strict';

!function ($) {

    /**
     * Motion module.
     * @module foundation.motion
     */

    var initClasses = ['mui-enter', 'mui-leave'];
    var activeClasses = ['mui-enter-active', 'mui-leave-active'];

    var Motion = {
        animateIn: function (element, animation, cb) {
            animate(true, element, animation, cb);
        },

        animateOut: function (element, animation, cb) {
            animate(false, element, animation, cb);
        }
    };

    function Move(duration, elem, fn) {
        var anim,
            prog,
            start = null;
        // console.log('called');

        function move(ts) {
            if (!start) start = window.performance.now();
            // console.log(start, ts);
            prog = ts - start;
            fn.apply(elem);

            if (prog < duration) {
                anim = window.requestAnimationFrame(move, elem);
            } else {
                window.cancelAnimationFrame(anim);
                elem.trigger('finished.zf.animate', [elem]).triggerHandler('finished.zf.animate', [elem]);
            }
        }
        anim = window.requestAnimationFrame(move);
    }

    /**
     * Animates an element in or out using a CSS transition class.
     * @function
     * @private
     * @param {Boolean} isIn - Defines if the animation is in or out.
     * @param {Object} element - jQuery or HTML object to animate.
     * @param {String} animation - CSS class to use.
     * @param {Function} cb - Callback to run when animation is finished.
     */
    function animate(isIn, element, animation, cb) {
        element = $(element).eq(0);

        if (!element.length) return;

        var initClass = isIn ? initClasses[0] : initClasses[1];
        var activeClass = isIn ? activeClasses[0] : activeClasses[1];

        // Set up the animation
        reset();

        element.addClass(animation).css('transition', 'none');

        requestAnimationFrame(function () {
            element.addClass(initClass);
            if (isIn) element.show();
        });

        // Start the animation
        requestAnimationFrame(function () {
            element[0].offsetWidth;
            element.css('transition', '').addClass(activeClass);
        });

        // Clean up the animation when it finishes
        element.one(Foundation.transitionend(element), finish);

        // Hides the element (for out animations), resets the element, and runs a callback
        function finish() {
            if (!isIn) element.hide();
            reset();
            if (cb) cb.apply(element);
        }

        // Resets transitions and removes motion-specific classes
        function reset() {
            element[0].style.transitionDuration = 0;
            element.removeClass(initClass + ' ' + activeClass + ' ' + animation);
        }
    }

    Foundation.Move = Move;
    Foundation.Motion = Motion;
}(jQuery);
'use strict';

!function ($) {

    var Nest = {
        Feather: function (menu) {
            var type = arguments.length <= 1 || arguments[1] === undefined ? 'zf' : arguments[1];

            //menu.attr('role', 'navigation');

            var items = menu.find('li').not(".tree-node-preloaded"),
                subMenuClass = 'is-' + type + '-submenu',
                subItemClass = subMenuClass + '-item',
                hasSubClass = 'is-' + type + '-submenu-parent';

            items.each(function () {
                var $item = $(this),
                    $sub = $item.children('ul');

                if ($sub.length) {
                    $item.children().children('a:first').addClass(hasSubClass).attr({
                        'aria-haspopup': true,
                    });

                    $sub.addClass('submenu ' + subMenuClass).attr({
                        'data-submenu': '',
                        'aria-hidden': false,
                        'role': 'menu'
                    });
                }

                if ($item.parent('[data-submenu]').length) {
                    $item.addClass('is-submenu-item ' + subItemClass);
                }
            });

            return;
        },
        Burn: function (menu, type) {
            var items = menu.find('li').removeAttr('tabindex'),
                subMenuClass = 'is-' + type + '-submenu',
                subItemClass = subMenuClass + '-item',
                hasSubClass = 'is-' + type + '-submenu-parent';

            menu.find('*').removeClass(subMenuClass + ' ' + subItemClass + ' ' + hasSubClass + ' is-submenu-item submenu is-active').removeAttr('data-submenu').css('display', '');

            // console.log(      menu.find('.' + subMenuClass + ', .' + subItemClass + ', .has-submenu, .is-submenu-item, .submenu, [data-submenu]')
            //           .removeClass(subMenuClass + ' ' + subItemClass + ' has-submenu is-submenu-item submenu')
            //           .removeAttr('data-submenu'));
            // items.each(function(){
            //   var $item = $(this),
            //       $sub = $item.children('ul');
            //   if($item.parent('[data-submenu]').length){
            //     $item.removeClass('is-submenu-item ' + subItemClass);
            //   }
            //   if($sub.length){
            //     $item.removeClass('has-submenu');
            //     $sub.removeClass('submenu ' + subMenuClass).removeAttr('data-submenu');
            //   }
            // });
        }
    };

    Foundation.Nest = Nest;
}(jQuery);
'use strict';

!function ($) {

    var MutationObserver = function () {
        var prefixes = ['WebKit', 'Moz', 'O', 'Ms', ''];
        for (var i = 0; i < prefixes.length; i++) {
            if (prefixes[i] + 'MutationObserver' in window) {
                return window[prefixes[i] + 'MutationObserver'];
            }
        }
        return false;
    }();

    var triggers = function (el, type) {
        el.data(type).split(' ').forEach(function (id) {
            $('#' + id)[type === 'close' ? 'trigger' : 'triggerHandler'](type + '.zf.trigger', [el]);
        });
    };
    // Elements with [data-open] will reveal a plugin that supports it when clicked.
    $(document).on('click.zf.trigger', '[data-open]', function () {
        triggers($(this), 'open');
    });

    // Elements with [data-close] will close a plugin that supports it when clicked.
    // If used without a value on [data-close], the event will bubble, allowing it to close a parent component.
    $(document).on('click.zf.trigger', '[data-close]', function () {
        var id = $(this).data('close');
        if (id) {
            triggers($(this), 'close');
        } else {
            $(this).trigger('close.zf.trigger');
        }
    });

    // Elements with [data-toggle] will toggle a plugin that supports it when clicked.
    $(document).on('click.zf.trigger', '[data-toggle]', function () {
        triggers($(this), 'toggle');
    });

    // Elements with [data-closable] will respond to close.zf.trigger events.
    $(document).on('close.zf.trigger', '[data-closable]', function (e) {
        e.stopPropagation();
        var animation = $(this).data('closable');

        if (animation !== '') {
            Foundation.Motion.animateOut($(this), animation, function () {
                $(this).trigger('closed.zf');
            });
        } else {
            $(this).fadeOut().trigger('closed.zf');
        }
    });

    $(document).on('focus.zf.trigger blur.zf.trigger', '[data-toggle-focus]', function () {
        var id = $(this).data('toggle-focus');
        $('#' + id).triggerHandler('toggle.zf.trigger', [$(this)]);
    });

    /**
    * Fires once after all other scripts have loaded
    * @function
    * @private
    */
    $(window).on('load', function () {
        checkListeners();
    });

    function checkListeners() {
        eventsListener();
        resizeListener();
        scrollListener();
        closemeListener();
    }

    //******** only fires this function once on load, if there's something to watch ********
    function closemeListener(pluginName) {
        var yetiBoxes = $('[data-yeti-box]'),
            plugNames = ['dropdown', 'tooltip', 'reveal'];

        if (pluginName) {
            if (typeof pluginName === 'string') {
                plugNames.push(pluginName);
            } else if (typeof pluginName === 'object' && typeof pluginName[0] === 'string') {
                plugNames.concat(pluginName);
            } else {
                console.error('Plugin names must be strings');
            }
        }
        if (yetiBoxes.length) {
            var listeners = plugNames.map(function (name) {
                return 'closeme.zf.' + name;
            }).join(' ');

            $(window).off(listeners).on(listeners, function (e, pluginId) {
                var plugin = e.namespace.split('.')[0];
                var plugins = $('[data-' + plugin + ']').not('[data-yeti-box="' + pluginId + '"]');

                plugins.each(function () {
                    var _this = $(this);

                    _this.triggerHandler('close.zf.trigger', [_this]);
                });
            });
        }
    }

    function resizeListener(debounce) {
        var timer = void 0,
            $nodes = $('[data-resize]');
        if ($nodes.length) {
            $(window).off('resize.zf.trigger').on('resize.zf.trigger', function (e) {
                if (timer) {
                    clearTimeout(timer);
                }

                timer = setTimeout(function () {

                    if (!MutationObserver) {
                        //fallback for IE 9
                        $nodes.each(function () {
                            $(this).triggerHandler('resizeme.zf.trigger');
                        });
                    }
                    //trigger all listening elements and signal a resize event
                    $nodes.attr('data-events', "resize");
                }, debounce || 10); //default time to emit resize event
            });
        }
    }

    function scrollListener(debounce) {
        var timer = void 0,
            $nodes = $('[data-scroll]');
        if ($nodes.length) {
            $(window).off('scroll.zf.trigger').on('scroll.zf.trigger', function (e) {
                if (timer) {
                    clearTimeout(timer);
                }

                timer = setTimeout(function () {

                    if (!MutationObserver) {
                        //fallback for IE 9
                        $nodes.each(function () {
                            $(this).triggerHandler('scrollme.zf.trigger');
                        });
                    }
                    //trigger all listening elements and signal a scroll event
                    $nodes.attr('data-events', "scroll");
                }, debounce || 10); //default time to emit scroll event
            });
        }
    }

    function eventsListener() {
        if (!MutationObserver) {
            return false;
        }
        var nodes = document.querySelectorAll('[data-resize], [data-scroll], [data-mutate]');

        //element callback
        var listeningElementsMutation = function (mutationRecordsList) {
            var $target = $(mutationRecordsList[0].target);
            //trigger the event handler for the element depending on type
            switch ($target.attr("data-events")) {

                case "resize":
                    $target.triggerHandler('resizeme.zf.trigger', [$target]);
                    break;

                case "scroll":
                    $target.triggerHandler('scrollme.zf.trigger', [$target, window.pageYOffset]);
                    break;

                // case "mutate" :
                // console.log('mutate', $target);
                // $target.triggerHandler('mutate.zf.trigger');
                //
                // //make sure we don't get stuck in an infinite loop from sloppy codeing
                // if ($target.index('[data-mutate]') == $("[data-mutate]").length-1) {
                //   domMutationObserver();
                // }
                // break;

                default:
                    return false;
                //nothing
            }
        };

        if (nodes.length) {
            //for each element that needs to listen for resizing, scrolling, (or coming soon mutation) add a single observer
            for (var i = 0; i <= nodes.length - 1; i++) {
                var elementObserver = new MutationObserver(listeningElementsMutation);
                elementObserver.observe(nodes[i], { attributes: true, childList: false, characterData: false, subtree: false, attributeFilter: ["data-events"] });
            }
        }
    }

    // ------------------------------------

    // [PH]
    // Foundation.CheckWatchers = checkWatchers;
    Foundation.IHearYou = checkListeners;
    // Foundation.ISeeYou = scrollListener;
    // Foundation.IFeelYou = closemeListener;
}(jQuery);

// function domMutationObserver(debounce) {
//   // !!! This is coming soon and needs more work; not active  !!! //
//   var timer,
//   nodes = document.querySelectorAll('[data-mutate]');
//   //
//   if (nodes.length) {
//     // var MutationObserver = (function () {
//     //   var prefixes = ['WebKit', 'Moz', 'O', 'Ms', ''];
//     //   for (var i=0; i < prefixes.length; i++) {
//     //     if (prefixes[i] + 'MutationObserver' in window) {
//     //       return window[prefixes[i] + 'MutationObserver'];
//     //     }
//     //   }
//     //   return false;
//     // }());
//
//
//     //for the body, we need to listen for all changes effecting the style and class attributes
//     var bodyObserver = new MutationObserver(bodyMutation);
//     bodyObserver.observe(document.body, { attributes: true, childList: true, characterData: false, subtree:true, attributeFilter:["style", "class"]});
//
//
//     //body callback
//     function bodyMutation(mutate) {
//       //trigger all listening elements and signal a mutation event
//       if (timer) { clearTimeout(timer); }
//
//       timer = setTimeout(function() {
//         bodyObserver.disconnect();
//         $('[data-mutate]').attr('data-events',"mutate");
//       }, debounce || 150);
//     }
//   }
// }
'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

!function ($) {

    /**
     * Drilldown module.
     * @module foundation.drilldown
     * @requires foundation.util.keyboard
     * @requires foundation.util.motion
     * @requires foundation.util.nest
     */

    var Drilldown = function () {
        /**
         * Creates a new instance of a drilldown menu.
         * @class
         * @param {jQuery} element - jQuery object to make into an accordion menu.
         * @param {Object} options - Overrides to the default plugin settings.
         */

        function Drilldown(element, options) {
            _classCallCheck(this, Drilldown);

            this.$element = element;
            this.options = $.extend({}, Drilldown.defaults, this.$element.data(), options);

            Foundation.Nest.Feather(this.$element, 'drilldown');

            this._init();

            Foundation.registerPlugin(this, 'Drilldown');
            Foundation.Keyboard.register('Drilldown', {
                'ENTER': 'open',
                'SPACE': 'open',
                'ARROW_RIGHT': 'next',
                'ARROW_UP': 'up',
                'ARROW_DOWN': 'down',
                'ARROW_LEFT': 'previous',
                'ESCAPE': 'close',
                'TAB': 'down',
                'SHIFT_TAB': 'up'
            });
        }

        /**
         * Initializes the drilldown by creating jQuery collections of elements
         * @private
         */


        _createClass(Drilldown, [{
            key: '_init',
            value: function _init() {
                this.$submenuAnchors = this.$element.find('li.is-drilldown-submenu-parent').children('a');
                this.$submenus = this.$submenuAnchors.parent('li').children('[data-submenu]');
                this.$menuItems = this.$element.find('li').not('.js-drilldown-back').find('a');

                this._prepareMenu();

                this._keyboardEvents();
            }

            /**
             * prepares drilldown menu by setting attributes to links and elements
             * sets a min height to prevent content jumping
             * wraps the element if not already wrapped
             * @private
             * @function
             */

        }, {
            key: '_prepareMenu',
            value: function _prepareMenu() {
                var _this = this;
                // if(!this.options.holdOpen){
                //   this._menuLinkEvents();
                // }
                this.$submenuAnchors.each(function () {
                    var $link = $(this);
                    var $sub = $link.parent();
                    if (_this.options.parentLink) {
                        $link.clone().prependTo($sub.children('[data-submenu]')).wrap('<li class="is-submenu-parent-item is-submenu-item is-drilldown-submenu-item" role="menu-item"></li>');
                    }
                    $link.data('savedHref', $link.attr('href')).removeAttr('href');
                    $link.children('[data-submenu]').attr({
                        'aria-hidden': true,
                        'tabindex': 0,
                        'role': 'menu'
                    });
                    _this._events($link);
                });
                this.$submenus.each(function () {
                    var $menu = $(this),
                        $back = $menu.find('.js-drilldown-back');
                    if (!$back.length) {
                        $menu.prepend(_this.options.backButton);
                    }
                    _this._back($menu);
                });
                if (!this.$element.parent().hasClass('is-drilldown')) {
                    this.$wrapper = $(this.options.wrapper).addClass('is-drilldown');
                    //this.$wrapper = this.$element.wrap(this.$wrapper).parent().css(this._getMaxDims());
                }
            }

            /**
             * Adds event handlers to elements in the menu.
             * @function
             * @private
             * @param {jQuery} $elem - the current menu item to add handlers to.
             */

        }, {
            key: '_events',
            value: function _events($elem) {
                var _this = this;

                $elem.off('click.zf.drilldown').on('click.zf.drilldown', function (e) {
                    if ($(e.target).parentsUntil('ul', 'li').hasClass('is-drilldown-submenu-parent')) {
                        e.stopImmediatePropagation();
                        e.preventDefault();
                    }

                    // if(e.target !== e.currentTarget.firstElementChild){
                    //   return false;
                    // }
                    _this._show($elem.parent('li'));

                    if (_this.options.closeOnClick) {
                        var $body = $('body');
                        $body.off('.zf.drilldown').on('click.zf.drilldown', function (e) {
                            if (e.target === _this.$element[0] || $.contains(_this.$element[0], e.target)) {
                                return;
                            }
                            e.preventDefault();
                            _this._hideAll();
                            $body.off('.zf.drilldown');
                        });
                    }
                });
            }

            /**
             * Adds keydown event listener to `li`'s in the menu.
             * @private
             */

        }, {
            key: '_keyboardEvents',
            value: function _keyboardEvents() {
                var _this = this;

                this.$menuItems.add(this.$element.find('.js-drilldown-back > a')).on('keydown.zf.drilldown', function (e) {

                    var $element = $(this),
                        $elements = $element.parent('li').parent('ul').children('li').children('a'),
                        $prevElement,
                        $nextElement;

                    $elements.each(function (i) {
                        if ($(this).is($element)) {
                            $prevElement = $elements.eq(Math.max(0, i - 1));
                            $nextElement = $elements.eq(Math.min(i + 1, $elements.length - 1));
                            return;
                        }
                    });

                    Foundation.Keyboard.handleKey(e, 'Drilldown', {
                        next: function () {
                            if ($element.is(_this.$submenuAnchors)) {
                                _this._show($element.parent('li'));
                                $element.parent('li').one(Foundation.transitionend($element), function () {
                                    $element.parent('li').find('ul li a').filter(_this.$menuItems).first().focus();
                                });
                                return true;
                            }
                        },
                        previous: function () {
                            _this._hide($element.parent('li').parent('ul'));
                            $element.parent('li').parent('ul').one(Foundation.transitionend($element), function () {
                                setTimeout(function () {
                                    $element.parent('li').parent('ul').parent('li').children('a').first().focus();
                                }, 1);
                            });
                            return true;
                        },
                        up: function () {
                            $prevElement.focus();
                            return true;
                        },
                        down: function () {
                            $nextElement.focus();
                            return true;
                        },
                        close: function () {
                            _this._back();
                            //_this.$menuItems.first().focus(); // focus to first element
                        },
                        open: function () {
                            if (!$element.is(_this.$menuItems)) {
                                // not menu item means back button
                                _this._hide($element.parent('li').parent('ul'));
                                $element.parent('li').parent('ul').one(Foundation.transitionend($element), function () {
                                    setTimeout(function () {
                                        $element.parent('li').parent('ul').parent('li').children('a').first().focus();
                                    }, 1);
                                });
                            } else if ($element.is(_this.$submenuAnchors)) {
                                _this._show($element.parent('li'));
                                $element.parent('li').one(Foundation.transitionend($element), function () {
                                    $element.parent('li').find('ul li a').filter(_this.$menuItems).first().focus();
                                });
                            }
                            return true;
                        },
                        handled: function (preventDefault) {
                            if (preventDefault) {
                                e.preventDefault();
                            }
                            e.stopImmediatePropagation();
                        }
                    });
                }); // end keyboardAccess
            }

            /**
             * Closes all open elements, and returns to root menu.
             * @function
             * @fires Drilldown#closed
             */

        }, {
            key: '_hideAll',
            value: function _hideAll() {
                var $elem = this.$element.find('.is-drilldown-submenu.is-active').addClass('is-closing');
                $elem.one(Foundation.transitionend($elem), function (e) {
                    $elem.removeClass('is-active is-closing');
                });
                /**
                 * Fires when the menu is fully closed.
                 * @event Drilldown#closed
                 */
                this.$element.trigger('closed.zf.drilldown');
            }

            /**
             * Adds event listener for each `back` button, and closes open menus.
             * @function
             * @fires Drilldown#back
             * @param {jQuery} $elem - the current sub-menu to add `back` event.
             */

        }, {
            key: '_back',
            value: function _back($elem) {
                var _this = this;
                $elem.off('click.zf.drilldown');
                $elem.children('.js-drilldown-back').on('click.zf.drilldown', function (e) {
                    e.stopImmediatePropagation();
                    // console.log('mouseup on back');
                    _this._hide($elem);
                });
            }

            /**
             * Adds event listener to menu items w/o submenus to close open menus on click.
             * @function
             * @private
             */

        }, {
            key: '_menuLinkEvents',
            value: function _menuLinkEvents() {
                var _this = this;
                this.$menuItems.not('.is-drilldown-submenu-parent').off('click.zf.drilldown').on('click.zf.drilldown', function (e) {
                    // e.stopImmediatePropagation();
                    setTimeout(function () {
                        _this._hideAll();
                    }, 0);
                });
            }

            /**
             * Opens a submenu.
             * @function
             * @fires Drilldown#open
             * @param {jQuery} $elem - the current element with a submenu to open, i.e. the `li` tag.
             */

        }, {
            key: '_show',
            value: function _show($elem) {
                $elem.children('[data-submenu]').addClass('is-active').attr("aria-hidden", false).css('display', 'block');
                $elem.children('a').attr('aria-expanded', true);
                /**
                 * Fires when the submenu has opened.
                 * @event Drilldown#open
                 */
                this.$element.trigger('open.zf.drilldown', [$elem]);
            }
        }, {
            key: '_hide',


            /**
             * Hides a submenu
             * @function
             * @fires Drilldown#hide
             * @param {jQuery} $elem - the current sub-menu to hide, i.e. the `ul` tag.
             */
            value: function _hide($elem) {
                var _this = this;
                $elem.addClass('is-closing').one(Foundation.transitionend($elem), function () {
                    $elem.removeClass('is-active is-closing').attr('aria-hidden', true).css('display', 'none');
                    $elem.parent().children('a').attr('aria-expanded', false);
                    $elem.blur();
                });
                /**
                 * Fires when the submenu has closed.
                 * @event Drilldown#hide
                 */
                $elem.trigger('hide.zf.drilldown', [$elem]);
            }

            /**
             * Iterates through the nested menus to calculate the min-height, and max-width for the menu.
             * Prevents content jumping.
             * @function
             * @private
             */

        }, {
            key: '_getMaxDims',
            value: function _getMaxDims() {
                var max = 0,
                    result = {};
                this.$submenus.add(this.$element).each(function () {
                    var numOfElems = $(this).children('li').length;
                    max = numOfElems > max ? numOfElems : max;
                });

                if (this.$menuItems.length > 0)
                    result['min-height'] = max * this.$menuItems[0].getBoundingClientRect().height + 'px';

                if (this.$element.length > 0)
                    result['max-width'] = this.$element[0].getBoundingClientRect().width + 'px';

                return result;
            }

            /**
             * Destroys the Drilldown Menu
             * @function
             */

        }, {
            key: 'destroy',
            value: function destroy() {
                this._hideAll();
                Foundation.Nest.Burn(this.$element, 'drilldown');
                this.$element.find('.js-drilldown-back, .is-submenu-parent-item').remove().end().find('.is-active, .is-closing, .is-drilldown-submenu').removeClass('is-active is-closing is-drilldown-submenu').end().find('[data-submenu]').removeAttr('aria-hidden tabindex role');
                this.$submenuAnchors.each(function () {
                    $(this).off('.zf.drilldown');
                });
                this.$element.find('a').each(function () {
                    var $link = $(this);
                    if ($link.data('savedHref')) {
                        $link.attr('href', $link.data('savedHref')).removeData('savedHref');
                    } else {
                        return;
                    }
                });
                Foundation.unregisterPlugin(this);
            }
        }]);

        return Drilldown;
    }();

    Drilldown.defaults = {
        /**
         * Markup used for JS generated back button. Prepended to submenu lists and deleted on `destroy` method, 'js-drilldown-back' class required. Remove the backslash (`\`) if copy and pasting.
         * @option
         * @example '<\li><\a>Back<\/a><\/li>'
         */
        backButton: '<li class="js-drilldown-back"><a tabindex="0">Back</a></li>',
        /**
         * Markup used to wrap drilldown menu. Use a class name for independent styling; the JS applied class: `is-drilldown` is required. Remove the backslash (`\`) if copy and pasting.
         * @option
         * @example '<\div class="is-drilldown"><\/div>'
         */
        wrapper: '<div></div>',
        /**
         * Adds the parent link to the submenu.
         * @option
         * @example false
         */
        parentLink: false,
        /**
         * Allow the menu to return to root list on body click.
         * @option
         * @example false
         */
        closeOnClick: false
        // holdOpen: false
    };

    // Window exports
    Foundation.plugin(Drilldown, 'Drilldown');
}(jQuery);
'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

!function ($) {

    /**
     * AccordionMenu module.
     * @module foundation.accordionMenu
     * @requires foundation.util.keyboard
     * @requires foundation.util.motion
     * @requires foundation.util.nest
     */

    var AccordionMenu = function () {
        /**
         * Creates a new instance of an accordion menu.
         * @class
         * @fires AccordionMenu#init
         * @param {jQuery} element - jQuery object to make into an accordion menu.
         * @param {Object} options - Overrides to the default plugin settings.
         */

        function AccordionMenu(element, options) {
            _classCallCheck(this, AccordionMenu);

            this.$element = element;
            this.options = $.extend({}, AccordionMenu.defaults, this.$element.data(), options);

            Foundation.Nest.Feather(this.$element, 'accordion');

            this._init();

            Foundation.registerPlugin(this, 'AccordionMenu');
            Foundation.Keyboard.register('AccordionMenu', {
                'ENTER': 'toggle',
                'SPACE': 'toggle',
                'ARROW_RIGHT': 'open',
                'ARROW_UP': 'up',
                'ARROW_DOWN': 'down',
                'ARROW_LEFT': 'close',
                'ESCAPE': 'closeAll',
                'TAB': 'down',
                'SHIFT_TAB': 'up'
            });
        }

        /**
         * Initializes the accordion menu by hiding all nested menus.
         * @private
         */


        _createClass(AccordionMenu, [{
            key: '_init',
            value: function _init() {
                this.$element.find('[data-submenu]').not('.is-active').slideUp(0); //.find('a').css('padding-left', '1rem');
                //this.$element.attr({
                //'role': 'tablist',
                //'aria-multiselectable': this.options.multiOpen
                //});

                // Task #165273 - exclude .tree-node-preloaded so that the menu can be initialized when everything is properly loaded
                this.$menuLinks = this.$element.find('.is-accordion-submenu-parent').not(".tree-node-preloaded");
                this.$menuLinks.each(function () {
                    var linkId = this.id || Foundation.GetYoDigits(6, 'acc-menu-link'),
                        $elem = $(this),
                        $sub = $elem.children('[data-submenu]'),
                        subId = $sub[0].id || Foundation.GetYoDigits(6, 'acc-menu'),
                        isActive = $sub.hasClass('is-active');
                    $elem.attr({
                        'aria-controls': subId,
                        'role': 'tab',
                        'id': linkId
                    });
                    $sub.attr({
                        'aria-labelledby': linkId,
                        'aria-hidden': !isActive,
                        'role': 'tabpanel',
                        'id': subId
                    });
                });
                var initPanes = this.$element.find('.is-active');
                if (initPanes.length) {
                    var _this = this;
                    initPanes.each(function () {
                        _this.down($(this));
                    });
                }
                this._events();
            }

            /**
             * Adds event handlers for items within the menu.
             * @private
             */

        }, {
            key: '_events',
            value: function _events() {
                var _this = this;

                this.$element.find('li').each(function () {
                    var $submenu = $(this).children('[data-submenu]');

                    if ($submenu.length) {
                        var $el = $(this).children('a');
                        if (_this.options.submenuToggle)
                            $el = $(this).find('a > .submenu-toggle-container');
                        $el.off('click.zf.accordionMenu').on('click.zf.accordionMenu', function (e) {
                            e.preventDefault();

                            _this.toggle($submenu);
                        });
                    }
                });
            }

            /**
             * Closes all panes of the menu.
             * @function
             */

        }, {
            key: 'hideAll',
            value: function hideAll() {
                this.$element.find('[data-submenu]').slideUp(this.options.slideSpeed);
            }

            /**
             * Toggles the open/close state of a submenu.
             * @function
             * @param {jQuery} $target - the submenu to toggle
             */

        }, {
            key: 'toggle',
            value: function toggle($target) {
                if (!$target.is(':animated')) {
                    if (!$target.is(':hidden')) {
                        this.up($target);
                    } else {
                        this.down($target);
                    }
                }
            }

            /**
             * Opens the sub-menu defined by `$target`.
             * @param {jQuery} $target - Sub-menu to open.
             * @fires AccordionMenu#down
             */

        }, {
            key: 'down',
            value: function down($target) {
                var _this = this;

                if (!this.options.multiOpen) {
                    this.up(this.$element.find('.is-active').not($target.parentsUntil(this.$element).add($target)));
                }
                
                $target.addClass('is-active').attr({ 'aria-hidden': false });

                //Foundation.Move(this.options.slideSpeed, $target, function() {
                $target.slideDown(_this.options.slideSpeed, function () {
                    /**
                     * Fires when the menu is done opening.
                     * @event AccordionMenu#down
                     */
                    _this.$element.trigger('down.zf.accordionMenu', [$target]);
                });
                //});
            }

            /**
             * Closes the sub-menu defined by `$target`. All sub-menus inside the target will be closed as well.
             * @param {jQuery} $target - Sub-menu to close.
             * @fires AccordionMenu#up
             */

        }, {
            key: 'up',
            value: function up($target) {
                var _this = this;
                //Foundation.Move(this.options.slideSpeed, $target, function(){
                $target.slideUp(_this.options.slideSpeed, function () {
                    /**
                     * Fires when the menu is done collapsing up.
                     * @event AccordionMenu#up
                     */
                    _this.$element.trigger('up.zf.accordionMenu', [$target]);
                });
                //});

                $target.find('[data-submenu]').slideUp(0).addBack().attr('aria-hidden', true);
                //$("[aria-controls='" + $menus.attr("id") + "']").attr({ "aria-expanded": false });;

            }

            /**
             * Destroys an instance of accordion menu.
             * @fires AccordionMenu#destroyed
             */

        }, {
            key: 'destroy',
            value: function destroy() {
                this.$element.find('[data-submenu]').slideDown(0).css('display', '');
                this.$element.find('a').off('click.zf.accordionMenu');

                Foundation.Nest.Burn(this.$element, 'accordion');
                Foundation.unregisterPlugin(this);
            }
        }]);

        return AccordionMenu;
    }();

    AccordionMenu.defaults = {
        /**
         * Amount of time to animate the opening of a submenu in ms.
         * @option
         * @example 250
         */
        slideSpeed: 250,
        /**
         * Allow the menu to have multiple open panes.
         * @option
         * @example true
         */
        multiOpen: true
    };

    // Window exports
    Foundation.plugin(AccordionMenu, 'AccordionMenu');
}(jQuery);
'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

!function ($) {

    /**
     * Magellan module.
     * @module foundation.magellan
     */

    var Magellan = function () {
        /**
         * Creates a new instance of Magellan.
         * @class
         * @fires Magellan#init
         * @param {Object} element - jQuery object to add the trigger to.
         * @param {Object} options - Overrides to the default plugin settings.
         */

        function Magellan(element, options) {
            _classCallCheck(this, Magellan);

            this.$element = element;
            this.options = $.extend({}, Magellan.defaults, this.$element.data(), options);

            this._init();

            Foundation.registerPlugin(this, 'Magellan');
        }

        /**
         * Initializes the Magellan plugin and calls functions to get equalizer functioning on load.
         * @private
         */


        _createClass(Magellan, [{
            key: '_init',
            value: function _init() {
                var id = this.$element[0].id || Foundation.GetYoDigits(6, 'magellan');
                var _this = this;
                this.$links = this.$element.find('a');
                var hashArray = this.$links.map(function () { return this.hash; })
                this.$targets = $('[data-magellan-target]').filter(function (i) {
                    return $.inArray('#' + encodeURIComponent(this.id), hashArray) != -1;
                });
                this.$element.attr({
                    'data-resize': id,
                    'data-scroll': id,
                    'id': id
                });
                this.$active = $();
                this.useScrollContainer = Foundation.MediaQuery.atLeast(this.options.scrollContainerOn) || window.matchMedia(this.options.scrollContainerOn).matches;
                this.scrollContainer = (this.useScrollContainer && this.options.scrollContainer) ? this.options.scrollContainer : window;

                this.scrollPos = parseInt($(this.scrollContainer).scrollTop(), 10);

                this._events(id);
            }

            /**
             * Calculates an array of pixel values that are the demarcation lines between locations on the page.
             * Can be invoked if new elements are added or the size of a location changes.
             * @function
             */

        }, {
            key: 'calcPoints',
            value: function calcPoints() {
                var _this = this,
                    body = this.scrollContainer == window ? document.body : $(this.scrollContainer)[0],
                    html = document.documentElement;

                this.points = [];
                this.winHeight = Math.round(Math.max(window.innerHeight, html.clientHeight));
                this.docHeight = Math.round(Math.max(body.scrollHeight, body.offsetHeight, html.clientHeight, html.scrollHeight, html.offsetHeight));

                this.$targets.each(function () {
                    var $tar = $(this),
                        pt = Math.round($tar.position().top - _this.options.threshold);
                    $tar.targetPoint = pt;
                    _this.points.push(pt);
                });
            }

            /**
             * Initializes events for Magellan.
             * @private
             */

        }, {
            key: '_events',
            value: function _events(id) {
                var _this = this,
                    scrollListener = this.scrollListener = 'scroll.zf.' + id;
                $body = $('html, body'),
                    opts = {
                        duration: _this.options.animationDuration,
                        easing: _this.options.animationEasing
                    };
                $(function () {
                    if (!_this.options) // if destroyed
                        return;

                    if (_this.options.deepLinking) {
                        if (location.hash) {
                            _this.scrollToLoc(location.hash);
                        }
                    }
                    _this.calcPoints();
                    _this._updateActive();

                    $(_this.scrollContainer).off(scrollListener).on(scrollListener, function (e) {
                        _this._updateActive();
                    });
                });

                this.$element.on({
                    'resizeme.zf.trigger': function () {
                        var oldScrollContainer = _this.scrollContainer;
                        _this.reflow();
                        if (oldScrollContainer != _this.scrollContainer)
                            _this._events(id);
                    },
                }).on('click.zf.magellan', 'a[href^="#"]', function (e) {
                    e.preventDefault();
                    var arrival = this.getAttribute('href');
                    _this.scrollToLoc(arrival);
                });
            }

            /**
             * Function to scroll to a given location on the page.
             * @param {String} loc - a properly formatted jQuery id selector. Example: '#foo'
             * @function
             */

        }, {
            key: 'scrollToLoc',
            value: function scrollToLoc(loc) {
                var self = this;
                var top = (this.scrollContainer == window) ? $(loc).offset().top : $(loc)[0].offsetTop;
                var scrollPos = Math.round(top - this.options.threshold / 2 - this.options.barOffset);
                var $container = this.scrollContainer == window ? $('html, body') : $(this.scrollContainer);

                $container.stop(true).animate({
                    scrollTop: scrollPos
                }, this.options.animationDuration, this.options.animationEasing, function () {
                    self._updateActive();
                });
            }

            /**
             * Calls necessary functions to update Magellan upon DOM change
             * @function
             */

        }, {
            key: 'reflow',
            value: function reflow() {
                var oldScrollContainer = this.scrollContainer;
                this.useScrollContainer = Foundation.MediaQuery.atLeast(this.options.scrollContainerOn) || window.matchMedia(this.options.scrollContainerOn).matches;

                this.scrollContainer = (this.useScrollContainer && this.options.scrollContainer) ? this.options.scrollContainer : window;
                if (oldScrollContainer != this.scrollContainer) {
                    $(oldScrollContainer).stop(true);
                    $(oldScrollContainer).off(this.scrollListener);
                }

                this.calcPoints();
                this._updateActive();
            }

            /**
             * Updates the visibility of an active location link, and updates the url hash for the page, if deepLinking enabled.
             * @private
             * @function
             * @fires Magellan#update
             */

        }, {
            key: '_updateActive',
            value: function _updateActive() /*evt, elem, scrollPos*/ {
                var winPos = /*scrollPos ||*/parseInt($(this.scrollContainer).scrollTop(), 10),
                    curIdx;

                if (winPos + this.winHeight === this.docHeight) {
                    curIdx = this.points.length - 1;
                } else if (winPos < this.points[0]) {
                    curIdx = 0;
                } else {
                    var isDown = this.scrollPos < winPos,
                        _this = this,
                        curVisible = this.points.filter(function (p, i) {
                            return isDown ? p - _this.options.barOffset <= winPos : p - _this.options.barOffset - _this.options.threshold <= winPos;
                        });
                    curIdx = curVisible.length ? curVisible.length - 1 : 0;
                }

                this.$active.removeClass(this.options.activeClass);
                this.$active = this.$links.eq(curIdx).addClass(this.options.activeClass);

                if (this.options.deepLinking) {
                    var hash = this.$active[0].getAttribute('href');
                    if (window.history.pushState) {
                        window.history.pushState(null, null, hash);
                    } else {
                        window.location.hash = hash;
                    }
                }

                this.scrollPos = winPos;
                /**
                 * Fires when magellan is finished updating to the new active element.
                 * @event Magellan#update
                 */
                this.$element.trigger('update.zf.magellan', [this.$active]);
            }

            /**
             * Destroys an instance of Magellan and resets the url of the window.
             * @function
             */

        }, {
            key: 'destroy',
            value: function destroy() {
                this.$element.off('.zf.trigger .zf.magellan').find('.' + this.options.activeClass).removeClass(this.options.activeClass);

                $(this.scrollContainer).off(this.scrollListener);

                if (this.options.deepLinking) {
                    var hash = this.$active[0].getAttribute('href');
                    window.location.hash.replace(hash, '');
                }

                Foundation.unregisterPlugin(this);
            }
        }]);

        return Magellan;
    }();

    /**
     * Default settings for plugin
     */


    Magellan.defaults = {
        /**
         * Amount of time, in ms, the animated scrolling should take between locations.
         * @option
         * @example 500
         */
        animationDuration: 500,
        /**
         * Animation style to use when scrolling between locations.
         * @option
         * @example 'ease-in-out'
         */
        animationEasing: 'linear',
        /**
         * Number of pixels to use as a marker for location changes.
         * @option
         * @example 50
         */
        threshold: 50,
        /**
         * Class applied to the active locations link on the magellan container.
         * @option
         * @example 'active'
         */
        activeClass: 'active',
        /**
         * Allows the script to manipulate the url of the current page, and if supported, alter the history.
         * @option
         * @example true
         */
        deepLinking: false,
        /**
         * Number of pixels to offset the scroll of the page on item click if using a sticky nav bar.
         * @option
         * @example 25
         */
        barOffset: 0
    };

    // Window exports
    Foundation.plugin(Magellan, 'Magellan');
}(jQuery);
'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

!function ($) {

    /**
     * OffCanvas module.
     * @module foundation.offcanvas
     * @requires foundation.util.mediaQuery
     * @requires foundation.util.triggers
     * @requires foundation.util.motion
     */

    var OffCanvas = function () {
        /**
         * Creates a new instance of an off-canvas wrapper.
         * @class
         * @fires OffCanvas#init
         * @param {Object} element - jQuery object to initialize.
         * @param {Object} options - Overrides to the default plugin settings.
         */

        function OffCanvas(element, options) {
            _classCallCheck(this, OffCanvas);

            this.$element = element;
            this.options = $.extend({}, OffCanvas.defaults, this.$element.data(), options);
            this.$lastTrigger = $();
            this.$triggers = $();

            this._init();
            this._events();

            Foundation.registerPlugin(this, 'OffCanvas');
        }

        /**
         * Initializes the off-canvas wrapper by adding the exit overlay (if needed).
         * @function
         * @private
         */


        _createClass(OffCanvas, [{
            key: '_init',
            value: function _init() {
                var id = this.$element.attr('id');

                this.$element.attr('aria-hidden', 'true');

                // Find triggers that affect this element and add aria-expanded to them
                //Bug #183306 - I think we don't want to change aria-expanded here; we want the parent link button to have the aria-expand attr for 508 compliance 

                this.$triggers = $(document).find('[data-open="' + id + '"], [data-close="' + id + '"], [data-toggle="' + id + '"]').attr('aria-controls', id); 

                // Add a close trigger over the body if necessary
                if (this.options.closeOnClick) {
                    if ($('.js-off-canvas-exit').length) {
                        this.$exiter = $('.js-off-canvas-exit');
                    } else {
                        var exiter = document.createElement('div');
                        exiter.setAttribute('class', 'js-off-canvas-exit');
                        $('[data-off-canvas-content]').append(exiter);

                        this.$exiter = $(exiter);
                    }
                }

                this.options.isRevealed = this.options.isRevealed || new RegExp(this.options.revealClass, 'g').test(this.$element[0].className);

                if (this.options.isRevealed) {
                    this.options.revealOn = this.options.revealOn || this.$element[0].className.match(/(reveal-for-medium|reveal-for-large)/g)[0].split('-')[2];
                    this._setMQChecker();
                }
                if (!this.options.transitionTime) {
                    this.options.transitionTime = parseFloat(window.getComputedStyle($('[data-off-canvas-wrapper]')[0]).transitionDuration) * 1000;
                }
            }

            /**
             * Adds event handlers to the off-canvas wrapper and the exit overlay.
             * @function
             * @private
             */

        }, {
            key: '_events',
            value: function _events() {
                this.$element.off('.zf.trigger .zf.offcanvas').on({
                    'open.zf.trigger': this.open.bind(this),
                    'close.zf.trigger': this.close.bind(this),
                    'toggle.zf.trigger': this.toggle.bind(this),
                    'keydown.zf.offcanvas': this._handleKeyboard.bind(this)
                });

                if (this.options.closeOnClick && this.$exiter.length) {
                    this.$exiter.on({ 'click.zf.offcanvas': this.close.bind(this) });
                }
            }

            /**
             * Applies event listener for elements that will reveal at certain breakpoints.
             * @private
             */

        }, {
            key: '_setMQChecker',
            value: function _setMQChecker() {
                var _this = this;

                $(window).on('changed.zf.mediaquery', function () {
                    if (Foundation.MediaQuery.atLeast(_this.options.revealOn)) {
                        _this.reveal(true);
                    } else {
                        _this.reveal(false);
                    }
                }).one('load.zf.offcanvas', function () {
                    if (Foundation.MediaQuery.atLeast(_this.options.revealOn)) {
                        _this.reveal(true);
                    }
                });
            }

            /**
             * Handles the revealing/hiding the off-canvas at breakpoints, not the same as open.
             * @param {Boolean} isRevealed - true if element should be revealed.
             * @function
             */

        }, {
            key: 'reveal',
            value: function reveal(isRevealed) {
                var $closer = this.$element.find('[data-close]');
                if (isRevealed) {
                    this.close();
                    this.isRevealed = true;
                    // if (!this.options.forceTop) {
                    //   var scrollPos = parseInt(window.pageYOffset);
                    //   this.$element[0].style.transform = 'translate(0,' + scrollPos + 'px)';
                    // }
                    // if (this.options.isSticky) { this._stick(); }
                    this.$element.off('open.zf.trigger toggle.zf.trigger');
                    if ($closer.length) {
                        $closer.hide();
                    }
                } else {
                    this.isRevealed = false;
                    // if (this.options.isSticky || !this.options.forceTop) {
                    //   this.$element[0].style.transform = '';
                    //   $(window).off('scroll.zf.offcanvas');
                    // }
                    this.$element.on({
                        'open.zf.trigger': this.open.bind(this),
                        'toggle.zf.trigger': this.toggle.bind(this)
                    });
                    if ($closer.length) {
                        $closer.show();
                    }
                }
            }

            /**
             * Opens the off-canvas menu.
             * @function
             * @param {Object} event - Event object passed from listener.
             * @param {jQuery} trigger - element that triggered the off-canvas to open.
             * @fires OffCanvas#opened
             */

        }, {
            key: 'open',
            value: function open(event, trigger) {
                if (this.$element.hasClass('is-open') || this.isRevealed) {
                    return;
                }
                var _this = this,
                    $body = $(document.body);
                this.$element.css('display', 'block');

                if (this.options.forceTop) {
                    $('body').scrollTop(0);
                }
                // window.pageYOffset = 0;

                // if (!this.options.forceTop) {
                //   var scrollPos = parseInt(window.pageYOffset);
                //   this.$element[0].style.transform = 'translate(0,' + scrollPos + 'px)';
                //   if (this.$exiter.length) {
                //     this.$exiter[0].style.transform = 'translate(0,' + scrollPos + 'px)';
                //   }
                // }
                /**
                 * Fires when the off-canvas menu opens.
                 * @event OffCanvas#opened
                 */
                Foundation.Move(this.options.transitionTime, this.$element, function () {
                    $('[data-off-canvas-wrapper]').addClass('is-off-canvas-open is-open-' + _this.options.position);

                    _this.$element.addClass('is-open');

                    // if (_this.options.isSticky) {
                    //   _this._stick();
                    // }
                });

                this.$triggers.attr('aria-expanded', 'true');
                this.$element.attr('aria-hidden', 'false').trigger('opened.zf.offcanvas');

                var $focusElement = this.$element.find('a.selected');
                if ($focusElement.length == 0) {
                    $focusElement = this.$element.find('a').first();
                }
                $focusElement.focus();

                if (trigger) {
                    this.$lastTrigger = trigger;
                }

                if (this.options.autoFocus) {
                    this.$element.one(Foundation.transitionend(this.$element), function () {
                        if (_this.options.closeOnClick) {
                            _this.$exiter.addClass('is-visible');
                        }

                        var $focusEl = _this.$element.find('a.selected');
                        if (!($focusEl.length > 0 && $focusEl.is(":visible"))) {
                            $focusEl = _this.$element.find('a, button').eq(0);
                        }
                        $focusEl.focus();
                    });
                }

                if (this.options.trapFocus) {
                    $('[data-off-canvas-content]').attr('tabindex', '-1');
                    this._trapFocus();
                }
            }

            /**
             * Traps focus within the offcanvas on open.
             * @private
             */

        }, {
            key: '_trapFocus',
            value: function _trapFocus() {
                var focusable = Foundation.Keyboard.findFocusable(this.$element),
                    first = focusable.eq(0),
                    last = focusable.eq(-1);

                focusable.off('.zf.offcanvas').on('keydown.zf.offcanvas', function (e) {
                    if (e.which === 9 || e.keycode === 9) {
                        if (e.target === last[0] && !e.shiftKey) {
                            e.preventDefault();
                            first.focus();
                        }
                        if (e.target === first[0] && e.shiftKey) {
                            e.preventDefault();
                            last.focus();
                        }
                    }
                });
            }

            /**
             * Allows the offcanvas to appear sticky utilizing translate properties.
             * @private
             */
            // OffCanvas.prototype._stick = function() {
            //   var elStyle = this.$element[0].style;
            //
            //   if (this.options.closeOnClick) {
            //     var exitStyle = this.$exiter[0].style;
            //   }
            //
            //   $(window).on('scroll.zf.offcanvas', function(e) {
            //     console.log(e);
            //     var pageY = window.pageYOffset;
            //     elStyle.transform = 'translate(0,' + pageY + 'px)';
            //     if (exitStyle !== undefined) { exitStyle.transform = 'translate(0,' + pageY + 'px)'; }
            //   });
            //   // this.$element.trigger('stuck.zf.offcanvas');
            // };
            /**
             * Closes the off-canvas menu.
             * @function
             * @param {Function} cb - optional cb to fire after closure.
             * @fires OffCanvas#closed
             */

        }, {
            key: 'close',
            value: function close(cb) {
                if (!this.$element.hasClass('is-open') || this.isRevealed) {
                    return;
                }

                var _this = this;

                this.$element.one(Foundation.transitionend(this.$element), function () {
                    $(this).css('display', 'none');
                });

                //  Foundation.Move(this.options.transitionTime, this.$element, function() {
                $('[data-off-canvas-wrapper]').removeClass('is-off-canvas-open is-open-' + _this.options.position);
                _this.$element.removeClass('is-open');
                // Foundation._reflow();
                // });
                this.$element.attr('aria-hidden', 'true')
                    /**
                     * Fires when the off-canvas menu opens.
                     * @event OffCanvas#closed
                     */
                    .trigger('closed.zf.offcanvas');
                // if (_this.options.isSticky || !_this.options.forceTop) {
                //   setTimeout(function() {
                //     _this.$element[0].style.transform = '';
                //     $(window).off('scroll.zf.offcanvas');
                //   }, this.options.transitionTime);
                // }
                if (this.options.closeOnClick) {
                    this.$exiter.removeClass('is-visible');
                }

                this.$triggers.attr('aria-expanded', 'false');
                if (this.options.trapFocus) {
                    $('[data-off-canvas-content]').removeAttr('tabindex');
                }
            }

            /**
             * Toggles the off-canvas menu open or closed.
             * @function
             * @param {Object} event - Event object passed from listener.
             * @param {jQuery} trigger - element that triggered the off-canvas to open.
             */

        }, {
            key: 'toggle',
            value: function toggle(event, trigger) {
                if (this.$element.hasClass('is-open')) {
                    this.close(event, trigger);
                } else {
                    this.open(event, trigger);
                }
            }

            /**
             * Handles keyboard input when detected. When the escape key is pressed, the off-canvas menu closes, and focus is restored to the element that opened the menu.
             * @function
             * @private
             */

        }, {
            key: '_handleKeyboard',
            value: function _handleKeyboard(event) {
                if (event.which !== 27) return;

                event.stopPropagation();
                event.preventDefault();
                this.close();
                this.$lastTrigger.focus();
            }

            /**
             * Destroys the offcanvas plugin.
             * @function
             */

        }, {
            key: 'destroy',
            value: function destroy() {
                this.close();
                this.$element.off('.zf.trigger .zf.offcanvas');
                this.$exiter.off('.zf.offcanvas');

                Foundation.unregisterPlugin(this);
            }
        }]);

        return OffCanvas;
    }();

    OffCanvas.defaults = {
        /**
         * Allow the user to click outside of the menu to close it.
         * @option
         * @example true
         */
        closeOnClick: true,

        /**
         * Amount of time in ms the open and close transition requires. If none selected, pulls from body style.
         * @option
         * @example 500
         */
        transitionTime: 0,

        /**
         * Direction the offcanvas opens from. Determines class applied to body.
         * @option
         * @example left
         */
        position: 'left',

        /**
         * Force the page to scroll to top on open.
         * @option
         * @example true
         */
        forceTop: false,

        /**
         * Allow the offcanvas to remain open for certain breakpoints.
         * @option
         * @example false
         */
        isRevealed: false,

        /**
         * Breakpoint at which to reveal. JS will use a RegExp to target standard classes, if changing classnames, pass your class with the `revealClass` option.
         * @option
         * @example reveal-for-large
         */
        revealOn: null,

        /**
         * Force focus to the offcanvas on open. If true, will focus the opening trigger on close.
         * @option
         * @example true
         */
        autoFocus: true,

        /**
         * Class used to force an offcanvas to remain open. Foundation defaults for this are `reveal-for-large` & `reveal-for-medium`.
         * @option
         * TODO improve the regex testing for this.
         * @example reveal-for-large
         */
        revealClass: 'reveal-for-',

        /**
         * Triggers optional focus trapping when opening an offcanvas. Sets tabindex of [data-off-canvas-content] to -1 for accessibility purposes.
         * @option
         * @example true
         */
        trapFocus: false
    };

    // Window exports
    Foundation.plugin(OffCanvas, 'OffCanvas');
}(jQuery);
'use strict';

var _createClass = function () { function defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } } return function (Constructor, protoProps, staticProps) { if (protoProps) defineProperties(Constructor.prototype, protoProps); if (staticProps) defineProperties(Constructor, staticProps); return Constructor; }; }();

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

!function ($) {

    /**
     * Sticky module.
     * @module foundation.sticky
     * @requires foundation.util.triggers
     * @requires foundation.util.mediaQuery
     */

    var Sticky = function () {
        /**
         * Creates a new instance of a sticky thing.
         * @class
         * @param {jQuery} element - jQuery object to make sticky.
         * @param {Object} options - options object passed when creating the element programmatically.
         */

        function Sticky(element, options) {
            _classCallCheck(this, Sticky);

            this.$element = element;
            this.options = $.extend({}, Sticky.defaults, this.$element.data(), options);

            this._init();

            Foundation.registerPlugin(this, 'Sticky');
        }

        /**
         * Initializes the sticky element by adding classes, getting/setting dimensions, breakpoints and attributes
         * @function
         * @private
         */


        _createClass(Sticky, [{
            key: '_init',
            value: function _init() {
                var $parent = this.$element.parent('[data-sticky-container]'),
                    id = this.$element[0].id || Foundation.GetYoDigits(6, 'sticky'),
                    _this = this;

                if (!$parent.length) {
                    this.wasWrapped = true;
                }
                this.$container = $parent.length ? $parent : $(this.options.container).wrapInner(this.$element);
                this.$container.addClass(this.options.containerClass);

                this.$element.addClass(this.options.stickyClass).attr({ 'data-resize': id });

                this.scrollCount = this.options.checkEvery;
                this.isStuck = false;

                function loaded() {
                    if (_this.options.anchor !== '') {
                        _this.$anchor = $('#' + _this.options.anchor);
                    } else {
                        _this._parsePoints();
                    }

                    _this._setSizes(function () {
                        _this._calc(false);
                    });
                    _this._events(id.split('-').reverse().join('-'));
                }

                if (document.readyState == "complete") { loaded(); }
                else {
                    $(window).one('load.zf.sticky', loaded);
                }
            }

            /**
             * If using multiple elements as anchors, calculates the top and bottom pixel values the sticky thing should stick and unstick on.
             * @function
             * @private
             */

        }, {
            key: '_parsePoints',
            value: function _parsePoints() {
                var top = this.options.topAnchor == "" ? 0 : this.options.topAnchor,
                    btm = this.options.btmAnchor == "" ? document.documentElement.scrollHeight : this.options.btmAnchor,
                    pts = [top, btm],
                    breaks = {};
                for (var i = 0, len = pts.length; i < len; i++) {
                    var pt;
                    if (typeof pts[i] === 'number') {
                        pt = pts[i];
                    } else {
                        var place = pts[i].split(':'),
                            anchor = $('#' + place[0]);

                        pt = anchor.offset().top;
                        if (place[1] && place[1].toLowerCase() === 'bottom') {
                            pt += anchor[0].getBoundingClientRect().height;
                        }
                    }
                    breaks[i] = pt;
                }

                this.points = breaks;
                return;
            }

            /**
             * Adds event handlers for the scrolling element.
             * @private
             * @param {String} id - psuedo-random id for unique scroll event listener.
             */

        }, {
            key: '_events',
            value: function _events(id) {
                var _this = this,
                    scrollListener = this.scrollListener = 'scroll.zf.' + id;
                if (this.isOn) {
                    return;
                }
                if (this.canStick) {
                    this.isOn = true;
                    var scrollContainer = (this.useScrollContainer && this.options.scrollContainer) ? this.options.scrollContainer : window;
                    $(scrollContainer).off(scrollListener).on(scrollListener, function (e) {
                        if (_this.scrollCount === 0) {
                            _this.scrollCount = _this.options.checkEvery;
                            _this._setSizes(function () {
                                _this._calc(false, window.pageYOffset);
                            });
                        } else {
                            _this.scrollCount--;
                            _this._setSizes(function () {
                                _this._calc(false, window.pageYOffset);
                            });
                        }
                    });
                }

                this.$element.off('resizeme.zf.trigger').on('resizeme.zf.trigger', function (e, el) {
                    _this._setSizes(function () {
                        _this._calc(false);
                        if (_this.canStick) {
                            if (!_this.isOn) {
                                _this._events(id);
                            }
                        } else if (_this.isOn) {
                            _this._pauseListeners(scrollListener);
                        }
                    });
                });
            }

            /**
             * Removes event handlers for scroll and change events on anchor.
             * @fires Sticky#pause
             * @param {String} scrollListener - unique, namespaced scroll listener attached to `window`
             */

        }, {
            key: '_pauseListeners',
            value: function _pauseListeners(scrollListener) {
                this.isOn = false;
                var scrollContainer = (this.useScrollContainer && this.options.scrollContainer) ? this.options.scrollContainer : window;
                $(scrollContainer).off(scrollListener);

                /**
                 * Fires when the plugin is paused due to resize event shrinking the view.
                 * @event Sticky#pause
                 * @private
                 */
                this.$element.trigger('pause.zf.sticky');
            }

            /**
             * Called on every `scroll` event and on `_init`
             * fires functions based on booleans and cached values
             * @param {Boolean} checkSizes - true if plugin should recalculate sizes and breakpoints.
             * @param {Number} scroll - current scroll position passed from scroll event cb function. If not passed, defaults to `window.pageYOffset`.
             */

        }, {
            key: '_calc',
            value: function _calc(checkSizes, scroll) {
                if (checkSizes) {
                    this._setSizes();
                }

                if (!this.canStick) {
                    if (this.isStuck) {
                        this._removeSticky(true);
                    }
                    return false;
                }

                if (!scroll) {
                    scroll = window.pageYOffset;
                }

                if (scroll >= this.topPoint) {
                    if (scroll <= this.bottomPoint) {
                        if (!this.isStuck) {
                            this._setSticky();
                        }
                    } else {
                        if (this.isStuck) {
                            this._removeSticky(false);
                        }
                    }
                } else {
                    if (this.isStuck) {
                        this._removeSticky(true);
                    }
                }
            }

            /**
             * Causes the $element to become stuck.
             * Adds `position: fixed;`, and helper classes.
             * @fires Sticky#stuckto
             * @function
             * @private
             */

        }, {
            key: '_setSticky',
            value: function _setSticky() {
                var _this = this,
                    stickTo = this.options.stickTo,
                    mrgn = stickTo === 'top' ? 'marginTop' : 'marginBottom',
                    notStuckTo = stickTo === 'top' ? 'bottom' : 'top',
                    css = {};

                css[mrgn] = this.options[mrgn] + 'em';
                css[stickTo] = this.options.stickToValue ? this.options.stickToValue : 0;
                css[notStuckTo] = 'auto';
                //css['left'] = this.$container.offset().left + parseInt(window.getComputedStyle(this.$container[0])["padding-left"], 10);
                this.isStuck = true;
                this.$element.removeClass('is-anchored is-at-' + notStuckTo).addClass('is-stuck is-at-' + stickTo).css(css)
                    /**
                     * Fires when the $element has become `position: fixed;`
                     * Namespaced to `top` or `bottom`, e.g. `sticky.zf.stuckto:top`
                     * @event Sticky#stuckto
                     */
                    .trigger('sticky.zf.stuckto:' + stickTo);
                this.$element.on("transitionend webkitTransitionEnd oTransitionEnd otransitionend MSTransitionEnd", function () {
                    if (this.options)
                        _this._setSizes();
                });
            }

            /**
             * Causes the $element to become unstuck.
             * Removes `position: fixed;`, and helper classes.
             * Adds other helper classes.
             * @param {Boolean} isTop - tells the function if the $element should anchor to the top or bottom of its $anchor element.
             * @fires Sticky#unstuckfrom
             * @private
             */

        }, {
            key: '_removeSticky',
            value: function _removeSticky(isTop) {
                var stickTo = this.options.stickTo,
                    stickToTop = stickTo === 'top',
                    css = {},
                    anchorPt = (this.points ? this.points[1] - this.points[0] : this.anchorHeight) - this.elemHeight,
                    mrgn = stickToTop ? 'marginTop' : 'marginBottom',
                    notStuckTo = stickToTop ? 'bottom' : 'top',
                    topOrBottom = isTop ? 'top' : 'bottom';

                css[mrgn] = 0;

                css['bottom'] = 'auto';
                if (isTop) {
                    css['top'] = 0;
                } else {
                    css['top'] = anchorPt;
                }

                css['left'] = '';
                this.isStuck = false;
                this.$element.removeClass('is-stuck is-at-' + stickTo).addClass('is-anchored is-at-' + topOrBottom).css(css)
                    /**
                     * Fires when the $element has become anchored.
                     * Namespaced to `top` or `bottom`, e.g. `sticky.zf.unstuckfrom:bottom`
                     * @event Sticky#unstuckfrom
                     */
                    .trigger('sticky.zf.unstuckfrom:' + topOrBottom);
            }

            /**
             * Sets the $element and $container sizes for plugin.
             * Calls `_setBreakPoints`.
             * @param {Function} cb - optional callback function to fire on completion of `_setBreakPoints`.
             * @private
             */

        }, {
            key: '_setSizes',
            value: function _setSizes(cb) {
                if (!this.options) return;

                this.canStick = Foundation.MediaQuery.atLeast(this.options.stickyOn) || window.matchMedia(this.options.stickyOn).matches;
                this.useScrollContainer = Foundation.MediaQuery.atLeast(this.options.scrollContainerOn) || window.matchMedia(this.options.scrollContainerOn).matches;
                if (!this.canStick) {
                    if (cb) {
                        cb();
                    }
                }
                var _this = this,
                    newElemWidth = this.$container[0].getBoundingClientRect().width,
                    comp = window.getComputedStyle(this.$container[0]),
                    pdng = parseInt(comp['padding-right'], 10);

                if (this.$anchor && this.$anchor.length) {
                    this.anchorHeight = this.$anchor[0].getBoundingClientRect().height;
                } else {
                    this._parsePoints();
                }

                this.$element.css({
                    'max-width': newElemWidth - pdng + 'px'
                });

                var newContainerHeight = this.$element[0].getBoundingClientRect().height || this.containerHeight;
                if (this.$element.css("display") == "none") {
                    newContainerHeight = 0;
                }
                this.containerHeight = newContainerHeight;
                this.$container.css({
                    'min-height': newContainerHeight
                });
                this.elemHeight = newContainerHeight;

                //if (this.isStuck) {
                //  this.$element.css({ "left": this.$container.offset().left + parseInt(comp['padding-left'], 10) });
                //}

                this._setBreakPoints(newContainerHeight, function () {
                    if (cb) {
                        cb();
                    }
                });
            }

            /**
             * Sets the upper and lower breakpoints for the element to become sticky/unsticky.
             * @param {Number} elemHeight - px value for sticky.$element height, calculated by `_setSizes`.
             * @param {Function} cb - optional callback function to be called on completion.
             * @private
             */

        }, {
            key: '_setBreakPoints',
            value: function _setBreakPoints(elemHeight, cb) {
                if (!this.canStick) {
                    if (cb) {
                        cb();
                    } else {
                        return false;
                    }
                }
                var mTop = emCalc(this.options.marginTop),
                    mBtm = emCalc(this.options.marginBottom),
                    topPoint = this.points ? this.points[0] : this.$anchor.offset().top,
                    bottomPoint = this.points ? this.points[1] : topPoint + this.anchorHeight,
                    winHeight = window.innerHeight;

                if (this.$anchor) {
                    topPoint = this.$anchor.offset().top || this.points[0],
                        bottomPoint = topPoint + this.anchorHeight || this.points[1]
                }

                if (this.options.stickTo === 'top') {
                    topPoint -= mTop;
                    bottomPoint -= elemHeight + mTop;
                } else if (this.options.stickTo === 'bottom') {
                    topPoint -= winHeight - (elemHeight + mBtm);
                    bottomPoint -= winHeight - mBtm;
                } else {
                    //this would be the stickTo: both option... tricky
                }

                this.topPoint = topPoint;
                this.bottomPoint = bottomPoint;

                if (cb) {
                    cb();
                }
            }

            /**
             * Destroys the current sticky element.
             * Resets the element to the top position first.
             * Removes event listeners, JS-added css properties and classes, and unwraps the $element if the JS added the $container.
             * @function
             */

        }, {
            key: 'destroy',
            value: function destroy() {
                this._removeSticky(true);

                this.$element.removeClass(this.options.stickyClass + ' is-anchored is-at-top').css({
                    height: '',
                    top: '',
                    bottom: '',
                    'max-width': ''
                }).off('resizeme.zf.trigger');
                if (this.$anchor && this.$anchor.length) {
                    this.$anchor.off('change.zf.sticky');
                }

                var scrollContainer = (this.useScrollContainer && this.options.scrollContainer) ? this.options.scrollContainer : window;
                $(scrollContainer).off(this.scrollListener);

                if (this.wasWrapped) {
                    this.$element.unwrap();
                } else {
                    this.$container.removeClass(this.options.containerClass).css({
                        height: ''
                    });
                }
                Foundation.unregisterPlugin(this);
            }
        }]);

        return Sticky;
    }();

    Sticky.defaults = {
        /**
         * Customizable container template. Add your own classes for styling and sizing.
         * @option
         * @example '&lt;div data-sticky-container class="small-6 columns"&gt;&lt;/div&gt;'
         */
        container: '<div data-sticky-container></div>',
        /**
         * Location in the view the element sticks to.
         * @option
         * @example 'top'
         */
        stickTo: 'top',
        /**
         * If anchored to a single element, the id of that element.
         * @option
         * @example 'exampleId'
         */
        anchor: '',
        /**
         * If using more than one element as anchor points, the id of the top anchor.
         * @option
         * @example 'exampleId:top'
         */
        topAnchor: '',
        /**
         * If using more than one element as anchor points, the id of the bottom anchor.
         * @option
         * @example 'exampleId:bottom'
         */
        btmAnchor: '',
        /**
         * Margin, in `em`'s to apply to the top of the element when it becomes sticky.
         * @option
         * @example 1
         */
        marginTop: 1,
        /**
         * Margin, in `em`'s to apply to the bottom of the element when it becomes sticky.
         * @option
         * @example 1
         */
        marginBottom: 1,
        /**
         * Breakpoint string that is the minimum screen size an element should become sticky.
         * @option
         * @example 'medium'
         */
        stickyOn: 'medium',
        /**
         * Class applied to sticky element, and removed on destruction. Foundation defaults to `sticky`.
         * @option
         * @example 'sticky'
         */
        stickyClass: 'sticky',
        /**
         * Class applied to sticky container. Foundation defaults to `sticky-container`.
         * @option
         * @example 'sticky-container'
         */
        containerClass: 'sticky-container',
        /**
         * Number of scroll events between the plugin's recalculating sticky points. Setting it to `0` will cause it to recalc every scroll event, setting it to `-1` will prevent recalc on scroll.
         * @option
         * @example 50
         */
        checkEvery: -1
    };

    /**
     * Helper function to calculate em values
     * @param Number {em} - number of em's to calculate into pixels
     */
    function emCalc(em) {
        return parseInt(window.getComputedStyle(document.body, null).fontSize, 10) * em;
    }

    // Window exports
    Foundation.plugin(Sticky, 'Sticky');
}(jQuery);