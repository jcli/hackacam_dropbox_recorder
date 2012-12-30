import os

from dropbox import client, rest, session

# XXX Fill in your consumer key and secret below
# You can find these at http://www.dropbox.com/developers/apps
APP_KEY = '93qck7duxnizmwx'
APP_SECRET = 'l4p63zop5zijol0'
ACCESS_TYPE = 'app_folder'  # should be 'dropbox' or 'app_folder' as configured for your app
class DropboxUploader():
    def __init__(self, app_key, app_secret):
        self.sess = StoredSession(app_key, app_secret, access_type=ACCESS_TYPE)
        self.api_client = client.DropboxClient(self.sess)
        self.current_path = ''
        self.sess.load_creds()

    def do_login(self):
        """log in to a Dropbox account"""
        try:
            self.sess.link()
        except rest.ErrorResponse, e:
#            self.stdout.write('Error: %s\n' % str(e))
            print(str(e))

class StoredSession(session.DropboxSession):
    """a wrapper around DropboxSession that stores a token to a file on disk"""
    TOKEN_FILE = "token_store.txt"

    def load_creds(self):
        try:
            stored_creds = open(self.TOKEN_FILE).read()
            self.set_token(*stored_creds.split('|'))
            print "[loaded access token]"
        except IOError:
            pass # don't worry if it's not there

    def write_creds(self, token):
        f = open(self.TOKEN_FILE, 'w')
        f.write("|".join([token.key, token.secret]))
        f.close()

    def delete_creds(self):
        os.unlink(self.TOKEN_FILE)

    def link(self):
        request_token = self.obtain_request_token()
        url = self.build_authorize_url(request_token)
        print "url:", url
        print "Please authorize in the browser. After you're done, press enter."
        raw_input()

        self.obtain_access_token(request_token)
        self.write_creds(self.token)

    def unlink(self):
        self.delete_creds()
        session.DropboxSession.unlink(self)
        
def main():
    if APP_KEY == '' or APP_SECRET == '':
        exit("You need to set your APP_KEY and APP_SECRET!")
    # term = DropboxTerm(APP_KEY, APP_SECRET)
    uploader = DropboxUploader(APP_KEY, APP_SECRET)
    # term.cmdloop()
    uploader.do_login()
    print("starting dropbox uploader")
    
if __name__ == '__main__':
    main()
