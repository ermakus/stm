from google.appengine.api import users
from google.appengine.ext import webapp
from google.appengine.ext.webapp.util import run_wsgi_app
from google.appengine.ext import db
from google.appengine.ext.webapp import template
import cgi
import os


VERSION_STATUS_UNKNOWN = 0        # bug may get fixed in an unknown version
VERSION_STATUS_ASSIGNED = 1       # bug will get fixed in a defined version
VERSION_STATUS_SUBMITTED = 2     # bug is fixed in a defined version, and the version has been submitted to the publisher
VERSION_STATUS_AVAILABLE = 3     # bug is fixed in a defined version, and the version is available for the customer
VERSION_STATUS_DISCONTINUED = 4  # version is no longer maintained, don't accept crash logs

class Report(db.Model):
    devid = db.StringProperty()
    xml = db.TextProperty()
    date = db.DateTimeProperty(auto_now_add=True)

class SitePage(webapp.RequestHandler):
    def render( self, name, params=None ):
        path = os.path.join( os.path.join(os.path.dirname(__file__), "pages"), name );
        self.response.out.write(template.render(path, params))
 
class CrashListPage( SitePage ):
    def get(self):
        user = users.get_current_user()
        if not user:
            self.redirect(users.create_login_url(self.request.uri))
        reports = db.GqlQuery("SELECT * FROM Report ORDER BY date DESC LIMIT 10")
        self.render('crashlist.html',  {'reports':reports} )

class CrashReportPage(webapp.RequestHandler):
    def get(self):
        self.response.headers['Content-Type'] = 'text/plain'
        self.response.out.write('OK')
    def post(self):
        report = Report()
        report.devid = self.request.get('id')
        report.xml = self.request.get('xmlstring')
        report.put()
        result = VERSION_STATUS_UNKNOWN;
        self.response.headers['Content-Type'] = 'text/xml'      
        self.response.out.write('<?xml version="1.0" encoding="UTF-8"?><result>' + str(result) + '</result>')

class BugReportPage( SitePage ):
    def get(self):
        self.response.headers['Content-Type'] = 'text/plain'
        self.response.out.write('OK')

    def post(self):
        report = Report()
        report.devid = self.request.get('email')
        report.xml = self.request.get('bug')
        report.put()
        self.render( 'thankyou.html' )


class BannerPage( SitePage ):
    def get(self):
	device = self.request.get('id')
        values = {
            'id': self.request.get('device')
        }
        self.render('banner.html', values )

class IndexPage( SitePage ):
    def get(self):
        self.render('index.html')

class FaqPage( SitePage ):
    def get(self):
        self.render('faq.html')

class ChangeListPage( SitePage ):
    def get(self):
        self.render('changelist.html')

class DonatePage( SitePage ):
    def get(self):
        self.render('donate.html')


application = webapp.WSGIApplication([
				     ('/', IndexPage ),
				     ('/index.html', IndexPage ),
				     ('/faq', FaqPage ),
				     ('/donate', DonatePage ),
				     ('/crashreport', CrashReportPage),
				     ('/bugreport', BugReportPage),
                                     ('/banner', BannerPage),
                                     ('/crashlist', CrashListPage),
                                     ('/changelist', ChangeListPage)
				     ],debug=True)

def main():
    run_wsgi_app(application)

if __name__ == "__main__":
    main()

